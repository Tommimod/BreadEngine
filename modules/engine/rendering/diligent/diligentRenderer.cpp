#include "diligentRenderer.h"

// Before everything else: GLEW insists on being the first to declare the GL entry points.
#include <GL/glew.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>

#include <EngineFactoryOpenGL.h>
#include <ShaderSourceFactoryUtils.hpp>
#include <Utilities/interface/DiligentFXShaderSourceStreamFactory.hpp>

#include "logger.h"
#include "rlgl.h"

namespace BreadEngine {
    /**
     * Reports what one texel of the colour target currently bound holds, after each pass of a
     * frame, for one frame. Off unless BREAD_FRAME_PROBE names a pixel as "x,y", counted from
     * the top left the way the capture tools in tools/render count.
     *
     * It answers the question that cost 7.e most of its debugging: not "is the frame wrong",
     * which a screenshot already says, but *which pass made it wrong*. A screenshot can only
     * show the end of the chain, so a pass that is correct and a pass that is corrupted
     * downstream of it look the same. Reading the target between passes separates them in one
     * run - in 7.e it showed the scene target still exactly right after the last bloom pass,
     * which moved the search downstream and closed it immediately.
     *
     * Compiled into every build rather than debug only: when it is off the cost is one branch
     * per pass, and a frame that is only wrong in a release build is exactly when it is
     * wanted. glReadPixels stalls the pipeline, which is why it runs for one frame.
     */
    void probeFrame(const char *afterPass, const Diligent::Uint32 targetHeight)
    {
        // Parsed once. -1 means the variable was absent or unreadable, and the probe stays off
        // for the process rather than re-testing it every pass.
        static int probeX = -2;
        static int probeY = 0;
        static int framesLeft = 1;

        if (probeX == -2)
        {
            probeX = -1;
            if (const char *request = std::getenv("BREAD_FRAME_PROBE"))
            {
                if (std::sscanf(request, "%d,%d", &probeX, &probeY) != 2) probeX = -1;
                else Logger::LogInfo("Frame probe at " + std::to_string(probeX) + "," + std::to_string(probeY));
            }
        }

        if (probeX < 0 || framesLeft <= 0) return;

        GLint framebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        // GL counts rows from the bottom and every tool that will be compared against this one
        // counts from the top, so the flip happens here rather than in the reader's head.
        float texel[4]{};
        glReadPixels(probeX, static_cast<GLint>(targetHeight) - 1 - probeY, 1, 1, GL_RGBA, GL_FLOAT, texel);

        Logger::LogInfo(std::string("Frame probe after ") + afterPass + ": " +
                        std::to_string(texel[0]) + ", " + std::to_string(texel[1]) + ", " +
                        std::to_string(texel[2]) + ", " + std::to_string(texel[3]) +
                        "  (fbo " + std::to_string(framebuffer) + ", gl error " + std::to_string(glGetError()) + ")");

        // The composite is the last pass of the chain, so counting the frame down there leaves
        // every earlier pass of that same frame already reported.
        if (std::strcmp(afterPass, "composite") == 0) --framesLeft;
    }

#ifndef NDEBUG
    /// One piece of GL state rlgl believes it owns, and the value it believes it holds.
    struct RaylibStateExpectation
    {
        const char *name;
        GLenum query;
        GLint expected;
    };

    /// What rlgl sets once in rlglInit and then only ever changes through its own wrappers.
    /// Every entry here is state some Diligent pipeline also sets, so every entry is something
    /// yieldToRaylib has to put back.
    constexpr RaylibStateExpectation RAYLIB_STATE[]{
        {"blending enabled", GL_BLEND, 1},
        {"blend source factor", GL_BLEND_SRC_RGB, GL_SRC_ALPHA},
        {"blend destination factor", GL_BLEND_DST_RGB, GL_ONE_MINUS_SRC_ALPHA},
        {"blend equation", GL_BLEND_EQUATION_RGB, GL_FUNC_ADD},
        {"depth test disabled", GL_DEPTH_TEST, 0},
        {"depth writes enabled", GL_DEPTH_WRITEMASK, 1},
        {"face culling enabled", GL_CULL_FACE, 1},
        {"culled face", GL_CULL_FACE_MODE, GL_BACK},
        {"front face winding", GL_FRONT_FACE, GL_CCW},
        {"sRGB conversion disabled", GL_FRAMEBUFFER_SRGB, 0},
        {"pixel unpack row length", GL_UNPACK_ROW_LENGTH, 0},
    };

    /**
     * Names every piece of GL state that no longer matches what rlgl believes it left behind.
     *
     * rlgl tracks this state in software and issues a call only when its own copy changes, so
     * anything a Diligent pipeline sets and yieldToRaylib does not put back is invisible until
     * it produces a wrong frame - somewhere that looks unrelated to rendering, because what
     * breaks is whatever raylib draws next. The migration's Invariants carry five rules of
     * exactly this shape, each one paid for in a bug; this is those rules made executable.
     *
     * Reported once per item rather than per frame: state that survives one frame survives all
     * of them, and a warning per frame is a warning nobody reads. Debug builds only - each
     * query is a pipeline stall, and the point is to fail during development, not to ship.
     */
    void verifyRaylibState(const Diligent::Uint32 samplerUnits)
    {
        // Which items have already been named. Function-local because the check has no other
        // state and the render thread is its only caller.
        static Diligent::Uint32 reported = 0;
        static bool samplersReported = false;
        static bool writeMaskReported = false;

        for (Diligent::Uint32 item = 0; item < std::size(RAYLIB_STATE); ++item)
        {
            if (reported & (1u << item)) continue;

            GLint actual = 0;
            glGetIntegerv(RAYLIB_STATE[item].query, &actual);
            if (actual == RAYLIB_STATE[item].expected) continue;

            reported |= 1u << item;
            Logger::LogWarning(std::string("GL state left for raylib is wrong: ") + RAYLIB_STATE[item].name +
                               " is " + std::to_string(actual) + ", raylib expects " +
                               std::to_string(RAYLIB_STATE[item].expected) +
                               ". Restore it in yieldToRaylib - rlgl will not.");
        }

        // Two more that do not fit the single-integer shape above.
        if (!writeMaskReported)
        {
            GLboolean mask[4]{};
            glGetBooleanv(GL_COLOR_WRITEMASK, mask);
            if (!mask[0] || !mask[1] || !mask[2] || !mask[3])
            {
                writeMaskReported = true;
                Logger::LogWarning("GL state left for raylib is wrong: a colour channel is masked off. "
                                   "A pipeline asking for COLOR_MASK_RGB leaves alpha unwritten for raylib too.");
            }
        }

        // Diligent binds a sampler object per texture unit and rlgl uses none, relying on each
        // texture's own parameters. One left bound overrides those, and its mipmapped
        // minification filter makes raylib's single-level textures incomplete - which samples
        // as opaque black and takes the whole UI with it.
        if (!samplersReported)
        {
            for (Diligent::Uint32 unit = 0; unit < samplerUnits; ++unit)
            {
                GLint sampler = 0;
                glGetIntegeri_v(GL_SAMPLER_BINDING, unit, &sampler);
                if (sampler == 0) continue;

                samplersReported = true;
                Logger::LogWarning("GL state left for raylib is wrong: a sampler object is still bound to texture unit " +
                                   std::to_string(unit) + ". raylib binds none and relies on each texture's own parameters.");
                break;
            }
        }
    }
#endif

    /// What raylib's own LoadRenderTexture stamps on a depth attachment; the field is unused
    /// for depth but DrawTexturePro-style paths still read it.
    constexpr int DEPTH_PIXEL_FORMAT = 19;

    void DiligentRenderer::initialize(const int sceneWidth, const int sceneHeight)
    {
        // Window.hWnd is deliberately left null. A non-null handle sends the Win32 GL backend
        // down its "create our own context" path, which calls SetPixelFormat on the window's
        // HDC - Windows rejects a second pixel format on the same HDC, and raylib has already
        // set one. Null takes the attach path, which adopts wglGetCurrentContext() as-is.
        const Diligent::EngineGLCreateInfo createInfo;
        Diligent::GetEngineFactoryOpenGL()->AttachToActiveGLContext(createInfo, &_device, &_context);

        if (!_device || !_context)
        {
            Logger::LogError("Diligent failed to attach to the active OpenGL context");
            return;
        }

        // The attached context is raylib's, so its version is whatever GLFW negotiated rather
        // than anything this engine asked for - worth knowing when a feature is unavailable.
        const auto &apiVersion = _device->GetDeviceInfo().APIVersion;
        Logger::LogInfo("Diligent attached to OpenGL " + std::to_string(apiVersion.Major) + "." + std::to_string(apiVersion.Minor));

        createSceneTarget(sceneWidth, sceneHeight);
        // Before the scene pipeline: the shadow arrays are static resources of it, so they have
        // to exist by the time that pipeline is built.
        _shadowPass.initializeMaps(_device, _context);
        // Also before the scene pipeline: the BRDF table is one of its static resources, and a
        // static variable can only be set while no binding has been created against it yet.
        _environment.initializeAmbient(_device, _context);
        createScenePipeline();
        // After it, because the depth pass uploads a caster's model matrix through the buffer
        // the scene pipeline created.
        _shadowPass.initializePipeline(_drawConstants);
        _screenSpace.initialize(_device, _context);
        _postChain.initialize(_device, _context);
        _overlayPass.initialize(_device, _context);
        _environment.initializeSky();

        // The BRDF table is integrated by a real pass, so this is the first work that draws
        // before a frame has ever been opened. raylib goes on to load its fonts and draw the
        // editor's first frame through the same context, and it would do both into the table's
        // framebuffer with the pipeline's state still applied.
        yieldToRaylib();
    }

    void DiligentRenderer::shutdown()
    {
        releaseSceneTarget();

        _draws.clear();
        _materials.clear();
        _meshes.clear();
        // Cleared before the pool it points into.
        _visibleLights.clear();
        _lights.clear();
        // Every slot the pool is about to drop may still have a decode running into it, and
        // the future does not wait on its own.
        _textures.forEachAlive([](TextureSlot &slot)
        {
            if (slot.decodeJob.valid()) slot.decodeJob.get();
        });
        _textures.clear();
        _materialFallbacks = {};
        _shadowPass.shutdown();
        _environment.shutdown();
        _screenSpace.shutdown();
        _postChain.shutdown();
        _overlayPass.shutdown();
        _scenePipeline.Release();
        _frameConstants.Release();
        _drawConstants.Release();
        _lightConstants.Release();

        _context.Release();
        _device.Release();
    }

    // --- frame ---

    void DiligentRenderer::createSceneTarget(const int width, const int height)
    {
        if (!_device || width <= 0 || height <= 0) return;
        if (_sceneColor &&
            _sceneColor->GetDesc().Width == static_cast<Diligent::Uint32>(width) &&
            _sceneColor->GetDesc().Height == static_cast<Diligent::Uint32>(height))
        {
            return;
        }

        releaseSceneTarget();

        Diligent::TextureDesc colorDesc;
        colorDesc.Name = "Scene color";
        colorDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        colorDesc.Width = static_cast<Diligent::Uint32>(width);
        colorDesc.Height = static_cast<Diligent::Uint32>(height);
        colorDesc.MipLevels = 1;
        colorDesc.Format = SCENE_COLOR_FORMAT;
        colorDesc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
        _device->CreateTexture(colorDesc, nullptr, &_sceneColor);

        Diligent::TextureDesc ambientDesc = colorDesc;
        ambientDesc.Name = "Scene ambient";
        _device->CreateTexture(ambientDesc, nullptr, &_sceneAmbient);

        Diligent::TextureDesc surfaceDesc = colorDesc;
        surfaceDesc.Name = "Scene surface";
        surfaceDesc.Format = SCENE_SURFACE_FORMAT;
        _device->CreateTexture(surfaceDesc, nullptr, &_sceneSurface);

        Diligent::TextureDesc depthDesc = colorDesc;
        depthDesc.Name = "Scene depth";
        depthDesc.Format = SCENE_DEPTH_FORMAT;
        depthDesc.BindFlags = Diligent::BIND_DEPTH_STENCIL | Diligent::BIND_SHADER_RESOURCE;
        _device->CreateTexture(depthDesc, nullptr, &_sceneDepth);

        Diligent::TextureDesc outputDesc = colorDesc;
        outputDesc.Name = "Scene output";
        outputDesc.Format = SCENE_OUTPUT_FORMAT;
        _device->CreateTexture(outputDesc, nullptr, &_sceneOutput);

        if (!_sceneColor || !_sceneAmbient || !_sceneSurface || !_sceneDepth || !_sceneOutput)
        {
            Logger::LogError("Diligent failed to create the scene render target");
            return;
        }

        // The composite pass reads this target one texel to one pixel, so point sampling is
        // not an approximation of the read - it is the read. Stated rather than left to the
        // backend's default, which filters and would soften the image by half a texel.
        Diligent::SamplerDesc sceneSampler;
        sceneSampler.MinFilter = sceneSampler.MagFilter = sceneSampler.MipFilter = Diligent::FILTER_TYPE_POINT;
        sceneSampler.AddressU = sceneSampler.AddressV = sceneSampler.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        Diligent::RefCntAutoPtr<Diligent::ISampler> sceneColorSampler;
        _device->CreateSampler(sceneSampler, &sceneColorSampler);
        _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sceneColorSampler);
        // The three targets the screen-space passes read are walked the same way: a texel at a
        // time, at the resolution they were written. A reconstructed position or a decoded
        // normal blended between two texels belongs to neither of the surfaces it came from.
        _sceneAmbient->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sceneColorSampler);
        _sceneSurface->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sceneColorSampler);
        _sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sceneColorSampler);

        // The GL backend's native handle is the texture name itself, which is all raylib
        // needs to treat these as its own.
        _overlay.id = rlLoadFramebuffer();
        _overlay.texture = Texture2D{
            .id = static_cast<unsigned int>(_sceneOutput->GetNativeHandle()),
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        _overlay.depth = Texture2D{
            .id = static_cast<unsigned int>(_sceneDepth->GetNativeHandle()),
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = DEPTH_PIXEL_FORMAT
        };

        rlFramebufferAttach(_overlay.id, _overlay.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(_overlay.id, _overlay.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferComplete(_overlay.id);
        rlDisableFramebuffer();
    }

    void DiligentRenderer::releaseSceneTarget()
    {
        if (_overlay.id != 0)
        {
            // rlUnloadFramebuffer deletes whatever texture it finds on the depth attachment,
            // and that one belongs to Diligent. Detaching first leaves it nothing to delete.
            rlFramebufferAttach(_overlay.id, 0, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
            rlUnloadFramebuffer(_overlay.id);
            _overlay = {};
        }

        _postChain.releaseTargets();
        _screenSpace.releaseTargets();
        _sceneColor.Release();
        _sceneAmbient.Release();
        _sceneSurface.Release();
        _sceneDepth.Release();
        _sceneOutput.Release();
    }

    void DiligentRenderer::resizeSceneTarget(const int width, const int height)
    {
        _hasExplicitTarget = true;
        createSceneTarget(width, height);
    }

    void DiligentRenderer::setOutputColorSpace(const OutputColorSpace colorSpace)
    {
        _outputEncoding = colorSpace == OutputColorSpace::Linear ? LINEAR_ENCODE_EXPONENT : GAMMA_ENCODE_EXPONENT;
    }

    void DiligentRenderer::setEnvironment(const EnvironmentSettings &settings)
    {
        _clearColor = settings.background.color;

        _environment.setSettings(settings);
        _screenSpace.setSettings(settings);
        _postChain.setSettings(settings);
    }

    void DiligentRenderer::beginScene(const CameraView &camera)
    {
        if (!_hasExplicitTarget) createSceneTarget(GetScreenWidth(), GetScreenHeight());

        _draws.clear();
        if (!_sceneColor) return;

        const auto &target = _sceneColor->GetDesc();
        const float aspect = static_cast<float>(target.Width) / static_cast<float>(target.Height);
        // The same clip range rlgl gives its own 3D passes. The editor's overlay draws into
        // this target's depth buffer through rlgl, so the two projections have to agree - and
        // that is also why the projection stays OpenGL's [-1, 1] depth convention.
        const auto nearPlane = static_cast<float>(rlGetCullDistanceNear());
        const auto farPlane = static_cast<float>(rlGetCullDistanceFar());

        Matrix projection;
        if (camera.projection == ProjectionType::Orthographic)
        {
            // Orthographic cameras spend fov as the visible height rather than an angle.
            const float halfHeight = camera.fov * 0.5f;
            projection = MatrixOrtho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, nearPlane, farPlane);
        }
        else
        {
            projection = MatrixPerspective(camera.fov * DEG2RAD, aspect, nearPlane, farPlane);
        }

        _viewProjection = MatrixMultiply(MatrixLookAt(camera.position, camera.target, camera.up), projection);
        _camera = camera;
    }

    void DiligentRenderer::endScene()
    {
        if (!_context) return;

        // raylib has been drawing through the same context since the last frame ended, so
        // whatever Diligent remembers about the GL state it left behind is stale.
        _context->InvalidateState();

        // Ahead of everything that binds the scene target, because baking a cube binds six
        // targets of its own - and ahead of the early return below, because a decode that has
        // landed should become usable whether or not there is a target to draw into this frame.
        _environment.finalizeCubemaps();

        if (!_sceneColor) return;

        // Ahead of the scene pass, which is the one that reads the maps. The cascades are fitted
        // to the frustum the frame will be seen through, so the shadow pass is handed the scene
        // target's aspect - it never renders into the target itself.
        const auto &sceneDesc = _sceneColor->GetDesc();
        selectVisibleLights(MAX_SCENE_LIGHTS);
        _shadowPass.render(_visibleLights, ShadowCasters{_draws, _meshes}, _camera,
                           static_cast<float>(sceneDesc.Width) / static_cast<float>(sceneDesc.Height));

        // The pass is bound and cleared here rather than in beginScene because the engine
        // pushes the environment - and with it the background colour - from a start-frame
        // system that runs after beginScene has already returned.
        Diligent::ITextureView *sceneTargets[]{
            _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET),
            _sceneAmbient->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET),
            _sceneSurface->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET)
        };
        auto *depthStencil = _sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
        _context->SetRenderTargets(static_cast<Diligent::Uint32>(std::size(sceneTargets)), sceneTargets, depthStencil,
                                   Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto clear = toSceneLinear(_clearColor, _outputEncoding);
        // Zero in the other two is what the background means to everything downstream: no
        // ambient light to take back, and a surface reflecting nothing. Both are then left
        // alone, because only the scene pass writes them.
        constexpr float empty[]{0.0f, 0.0f, 0.0f, 0.0f};
        _context->ClearRenderTarget(sceneTargets[0], &clear.x, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearRenderTarget(sceneTargets[1], empty, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearRenderTarget(sceneTargets[2], empty, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(depthStencil, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto probeHeight = sceneDesc.Height;
        probeFrame("clear", probeHeight);
        submitDraws();
        probeFrame("geometry", probeHeight);

        // Everything past the scene pass writes colour alone, and a pipeline's target count is
        // part of it - so the background is drawn against one target rather than three.
        _context->SetRenderTargets(1, sceneTargets, depthStencil, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _environment.drawSkybox(_camera, _viewProjection);
        probeFrame("background", probeHeight);
        _screenSpace.drawAmbientOcclusion(_sceneColor, _sceneAmbient, _sceneDepth, _sceneSurface, _camera, _viewProjection);
        probeFrame("occlusion", probeHeight);
        _screenSpace.drawReflections(_sceneColor, _sceneDepth, _sceneSurface, _camera, _viewProjection,
                                     _environment.ambientLookup());
        probeFrame("reflections", probeHeight);
        _postChain.drawFog(_sceneColor, _sceneDepth, _camera, _viewProjection, _outputEncoding);
        probeFrame("fog", probeHeight);
        _postChain.drawBloom(_sceneColor);
        probeFrame("bloom", probeHeight);
        auto *finalColor = _postChain.drawDepthOfField(_sceneColor, _sceneDepth, _camera, _viewProjection);
        probeFrame("dof", probeHeight);
        _postChain.composite(finalColor, _sceneOutput, _outputEncoding);
        probeFrame("composite", probeHeight);

        yieldToRaylib();

        // Without a caller-sized target the scene is the frame, so it goes to the backbuffer
        // raylib is currently drawing into.
        if (!_hasExplicitTarget)
        {
            drawSceneTexture(Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
        }
    }

    void uploadConstants(Diligent::IDeviceContext *context, Diligent::IBuffer *buffer, const void *data, const size_t size)
    {
        // A mapped constant buffer is driver memory with nothing behind it, so writing past the
        // end corrupts whatever the driver keeps there and crashes somewhere else entirely,
        // frames later. Refusing the write turns that into one legible message.
        if (size > buffer->GetDesc().Size)
        {
            Logger::LogError(std::string("Constants for '") + buffer->GetDesc().Name + "' are larger than the buffer holding them");
            return;
        }

        void *mapped = nullptr;
        context->MapBuffer(buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped);
        if (mapped == nullptr) return;

        std::memcpy(mapped, data, size);
        context->UnmapBuffer(buffer, Diligent::MAP_WRITE);
    }

    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> createShaderSources()
    {
        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> engineSources;
        const std::string shaderDirectory = std::string(GetApplicationDirectory()) + SHADER_DIRECTORY;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(shaderDirectory.c_str(), &engineSources);

        // DiligentFX's .fxh files are compiled into the library rather than shipped next to the
        // executable, so an #include of one only resolves through its own factory.
        return Diligent::CreateCompoundShaderSourceFactory(
            {&Diligent::DiligentFXShaderSourceStreamFactory::GetInstance(), engineSources});
    }

    void restoreRaylibPixelStore()
    {
        // Diligent leaves GL_UNPACK_ROW_LENGTH at the stride of whatever it uploaded last, and
        // raylib sets only the alignment before its own uploads - it has always been able to
        // assume the default row length. Left dirty, raylib's next upload walks its source at
        // Diligent's stride: with a 1x1 texture behind us that is one pixel per row, which
        // reduces the editor's font atlas to nothing and makes every glyph render blank.
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    void DiligentRenderer::yieldToRaylib()
    {
        _context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->InvalidateState();
        _context->Flush();

        // rlgl tracks the GL state it expects in software and only issues the calls it thinks
        // are needed, so every piece of state a pipeline sets differently has to be put back by
        // hand. Depth writes still match rlgl's own defaults; blending, the depth test and
        // both halves of the face-culling state do not.
        rlEnableColorBlend();
        // Turning blending back on says nothing about which function it runs, and that is the
        // half rlgl caches hardest: it issues glBlendFunc once at startup and again only when
        // its own blend mode changes, which nothing here ever makes it do. Every function
        // Diligent set before the bloom combine happened to be this one, so it went unnoticed;
        // the additive and screen combines are the first that are not, and rlgl would blit the
        // frame and draw its whole UI through whichever one was left behind.
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        rlDisableDepthTest();
        // Depth writes were rlgl's default by coincidence for as long as no pipeline turned
        // them off. The overlay's do: it tests the scene's depth and writes none of its own,
        // so that what the editor draws over it is not occluded by it.
        glDepthMask(GL_TRUE);
        rlEnableBackfaceCulling();
        // Which winding faces front is the other half, and restoring the cull test without it
        // is worse than restoring neither: raylib winds counter-clockwise, so a pipeline that
        // left GL_CW makes every triangle raylib draws a back face. Lines are not subject to
        // the cull test at all, which is what makes that failure look like "only lines draw".
        glFrontFace(GL_CCW);

        // Diligent binds a sampler object per texture unit; rlgl uses none and relies on each
        // texture's own parameters. A sampler left bound overrides those, and its mipmapped
        // minification filter makes raylib's single-level textures incomplete - which samples
        // as opaque black, taking the whole editor UI with it.
        for (Diligent::Uint32 unit = 0; unit < MATERIAL_TEXTURE_COUNT; ++unit) glBindSampler(unit, 0);

        // A pass writing into the output texture masks alpha off, because that channel is what
        // raylib blends the finished frame into the window through and nothing in the pass has
        // an opinion about it. The mask is global state, so rlgl would inherit it and draw its
        // whole UI with no alpha at all.
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // Diligent enables sRGB framebuffer conversion once at device creation and leaves it
        // on. It is a no-op for its own non-sRGB targets, but raylib's colours are already
        // encoded, so leaving it on would gamma them a second time on the way to the window.
        glDisable(GL_FRAMEBUFFER_SRGB);

#ifndef NDEBUG
        // Last, so it sees exactly what raylib will. Everything above is a rule that was once
        // a bug; this is what makes the next one of them fail on the first run instead of the
        // twelfth.
        verifyRaylibState(MATERIAL_TEXTURE_COUNT);
#endif
    }

    void DiligentRenderer::beginSceneOverlay()
    {
        if (_overlay.id == 0) return;

        BeginTextureMode(_overlay);
    }

    void DiligentRenderer::endSceneOverlay()
    {
        if (_overlay.id == 0) return;

        EndTextureMode();
    }

    OverlayEffectHandle DiligentRenderer::createOverlayEffect(const OverlayEffectDesc &desc)
    {
        return _overlayPass.createEffect(desc);
    }

    void DiligentRenderer::destroyOverlayEffect(const OverlayEffectHandle handle)
    {
        _overlayPass.destroyEffect(handle);
    }

    OverlayMeshHandle DiligentRenderer::createOverlayMesh(const OverlayMeshData &data)
    {
        return _overlayPass.createMesh(data);
    }

    void DiligentRenderer::destroyOverlayMesh(const OverlayMeshHandle handle)
    {
        _overlayPass.destroyMesh(handle);
    }

    void DiligentRenderer::beginOverlay()
    {
        if (!_context || !_sceneOutput) return;

        // endScene handed the context back to raylib, which has been drawing through it since,
        // so what Diligent remembers about the GL state it left behind is stale again.
        _context->InvalidateState();
        _overlayPass.begin(_camera, _viewProjection, _sceneOutput, _sceneDepth);
    }

    void DiligentRenderer::drawOverlay(const OverlayDrawDesc &draw)
    {
        // Null wherever the draw names no texture: an effect whose shader reads none ignores
        // it, and one that reads a texture it was not given is a client-side bug the binding
        // reports for itself.
        _overlayPass.draw(draw, textureView(draw.texture));
    }

    void DiligentRenderer::endOverlay()
    {
        if (!_context) return;

        yieldToRaylib();
    }

    void DiligentRenderer::drawSceneTexture(const Rectangle destination)
    {
        if (_overlay.id == 0) return;

        const auto width = static_cast<float>(_overlay.texture.width);
        const auto height = static_cast<float>(_overlay.texture.height);
        DrawTexturePro(_overlay.texture, Rectangle{0, 0, width, -height}, destination, Vector2{0, 0}, 0, WHITE);
    }
} // namespace BreadEngine
