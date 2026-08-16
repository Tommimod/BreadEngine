#include "diligentRenderer.h"

// Before everything else: GLEW insists on being the first to declare the GL entry points.
#include <GL/glew.h>

#include <cstring>
#include <string>

#include <EngineFactoryOpenGL.h>
#include <Sampler.h>
#include <Shader.h>

#include "logger.h"
#include "raymath.h"
#include "rlgl.h"

#include "../geometry/primitiveGenerator.h"
#include "utils/workerPool.h"

namespace BreadEngine {
    /// What raylib's own LoadRenderTexture stamps on a depth attachment; the field is unused
    /// for depth but DrawTexturePro-style paths still read it.
    constexpr int DEPTH_PIXEL_FORMAT = 19;

    /// The scene target's formats are fixed, so every pipeline that renders into it can be
    /// built against them once instead of being rebuilt when the target is resized.
    constexpr Diligent::TEXTURE_FORMAT SCENE_COLOR_FORMAT = Diligent::TEX_FORMAT_RGBA8_UNORM;
    constexpr Diligent::TEXTURE_FORMAT SCENE_DEPTH_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;

    /// Where the engine's shader sources sit relative to the executable.
    constexpr const char *SHADER_DIRECTORY = "shaders";

    /// Shader variable names of the material texture slots, in MaterialData's own order.
    constexpr const char *MATERIAL_TEXTURE_NAMES[]{"g_Albedo", "g_Normal", "g_Orm", "g_Emission"};

    /**
     * Mirrors scene.vsh's cbuffers. Everything is a float4 or a float4x4 on purpose: those
     * are the only members whose std140 placement is the same as their placement here, so the
     * struct and the shader cannot drift apart over padding.
     */
    struct SceneFrameConstants
    {
        float16 viewProjection;
        Vector4 cameraPosition;
        Vector4 lightDirection;
        Vector4 lightColor;
        Vector4 ambientColor;
        Vector4 outputEncoding;
    };

    struct SceneDrawConstants
    {
        float16 model;
        float16 normalMatrix;
    };

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
        createScenePipeline();
    }

    void DiligentRenderer::shutdown()
    {
        releaseSceneTarget();

        _draws.clear();
        _meshes.clear();
        _lights.clear();
        // Every slot the pool is about to drop may still have a decode running into it, and
        // the future does not wait on its own.
        _textures.forEachAlive([](TextureSlot &slot)
        {
            if (slot.decodeJob.valid()) slot.decodeJob.get();
        });
        _textures.clear();
        _materialTextures = {};
        _sceneResources.Release();
        _scenePipeline.Release();
        _frameConstants.Release();
        _drawConstants.Release();

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

        Diligent::TextureDesc depthDesc = colorDesc;
        depthDesc.Name = "Scene depth";
        depthDesc.Format = SCENE_DEPTH_FORMAT;
        depthDesc.BindFlags = Diligent::BIND_DEPTH_STENCIL;
        _device->CreateTexture(depthDesc, nullptr, &_sceneDepth);

        if (!_sceneColor || !_sceneDepth)
        {
            Logger::LogError("Diligent failed to create the scene render target");
            return;
        }

        // The GL backend's native handle is the texture name itself, which is all raylib
        // needs to treat these as its own.
        _overlay.id = rlLoadFramebuffer();
        _overlay.texture = Texture2D{
            .id = static_cast<unsigned int>(_sceneColor->GetNativeHandle()),
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

        _sceneColor.Release();
        _sceneDepth.Release();
    }

    void DiligentRenderer::createScenePipeline()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

        constantsDesc.Name = "Frame constants";
        constantsDesc.Size = sizeof(SceneFrameConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_frameConstants);
        constantsDesc.Name = "Draw constants";
        constantsDesc.Size = sizeof(SceneDrawConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_drawConstants);

        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shaderSources;
        const std::string shaderDirectory = std::string(GetApplicationDirectory()) + SHADER_DIRECTORY;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(shaderDirectory.c_str(), &shaderSources);

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;
        // Combined texture samplers - each Texture2D paired with a SamplerState named after
        // it plus "_sampler" - are what a GL device wants, and what DiligentFX asks for on one.

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Scene VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "scene.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        shaderInfo.Desc = {"Scene PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "scene.psh";
        _device->CreateShader(shaderInfo, &pixelShader);

        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the scene shaders from " + shaderDirectory);
            return;
        }

        // Slot, offset and stride are left to Diligent: one interleaved buffer in MeshVertex's
        // own order is the only layout the geometry side produces.
        constexpr Diligent::LayoutElement vertexLayout[]{
            {0, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // position
            {1, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // normal
            {2, 0, 2, Diligent::VT_FLOAT32, Diligent::False}, // uv
            {3, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // tangent
        };

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Scene opaque";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SCENE_COLOR_FORMAT;
        graphics.DSVFormat = SCENE_DEPTH_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
        // Diligent defaults front faces to clockwise; the engine's geometry is wound the way
        // rlgl winds its own, so the overlay pass and the scene agree on which side is front.
        graphics.RasterizerDesc.FrontCounterClockwise = Diligent::True;
        graphics.DepthStencilDesc.DepthEnable = Diligent::True;
        graphics.InputLayout.LayoutElements = vertexLayout;
        graphics.InputLayout.NumElements = static_cast<Diligent::Uint32>(std::size(vertexLayout));

        // The material textures change from draw to draw, which is what DYNAMIC means here;
        // everything else - the two constant buffers - stays bound for the pipeline's life.
        Diligent::ShaderResourceVariableDesc materialVariables[MATERIAL_TEXTURE_COUNT];
        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            materialVariables[slot] = {Diligent::SHADER_TYPE_PIXEL, MATERIAL_TEXTURE_NAMES[slot],
                                       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
        }
        pipelineInfo.PSODesc.ResourceLayout.Variables = materialVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(MATERIAL_TEXTURE_COUNT);

        _device->CreateGraphicsPipelineState(pipelineInfo, &_scenePipeline);
        if (!_scenePipeline)
        {
            Logger::LogError("Diligent failed to create the scene pipeline state");
            return;
        }

        // Both shaders read the frame block, and a static variable is per shader stage, so
        // binding it once for the vertex stage would leave the pixel stage's copy unset.
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "FrameConstants")->Set(_frameConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "FrameConstants")->Set(_frameConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "DrawConstants")->Set(_drawConstants);
        _scenePipeline->CreateShaderResourceBinding(&_sceneResources, true);

        createMaterialFallbacks();
        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            // Resolved once: the lookup is by name, and the draw loop runs it per material.
            _materialTextures[slot].variable = _sceneResources->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, MATERIAL_TEXTURE_NAMES[slot]);
        }
    }

    void DiligentRenderer::restoreRaylibPixelStore()
    {
        // Diligent leaves GL_UNPACK_ROW_LENGTH at the stride of whatever it uploaded last, and
        // raylib sets only the alignment before its own uploads - it has always been able to
        // assume the default row length. Left dirty, raylib's next upload walks its source at
        // Diligent's stride: with a 1x1 texture behind us that is one pixel per row, which
        // reduces the editor's font atlas to nothing and makes every glyph render blank.
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    void DiligentRenderer::createMaterialFallbacks()
    {
        // In MATERIAL_TEXTURE_NAMES' order: white albedo, a flat tangent-space normal,
        // occlusion 1 / roughness 1 / metalness 0, and no emission.
        constexpr Diligent::Uint32 fallbackPixels[MATERIAL_TEXTURE_COUNT]{0xFFFFFFFF, 0xFFFF8080, 0xFF00FFFF, 0xFF000000};

        Diligent::TextureDesc desc;
        desc.Name = "Material fallback";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
        desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            Diligent::TextureSubResData level{&fallbackPixels[slot], sizeof(Diligent::Uint32)};
            const Diligent::TextureData data{&level, 1};
            _device->CreateTexture(desc, &data, &_materialTextures[slot].fallback);
        }

        restoreRaylibPixelStore();
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
        _cameraPosition = camera.position;
    }

    void DiligentRenderer::endScene()
    {
        if (!_sceneColor) return;

        // raylib has been drawing through the same context since the last frame ended, so
        // whatever Diligent remembers about the GL state it left behind is stale.
        _context->InvalidateState();

        // The pass is bound and cleared here rather than in beginScene because the engine
        // pushes the environment - and with it the background colour - from a start-frame
        // system that runs after beginScene has already returned.
        auto *renderTarget = _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        auto *depthStencil = _sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
        _context->SetRenderTargets(1, &renderTarget, depthStencil, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto clear = ColorNormalize(_clearColor);
        _context->ClearRenderTarget(renderTarget, &clear.x, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(depthStencil, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        submitDraws();

        yieldToRaylib();

        // Without a caller-sized target the scene is the frame, so it goes to the backbuffer
        // raylib is currently drawing into.
        if (!_hasExplicitTarget)
        {
            drawSceneTexture(Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
        }
    }

    void DiligentRenderer::uploadConstants(Diligent::IBuffer *buffer, const void *data, const size_t size)
    {
        void *mapped = nullptr;
        _context->MapBuffer(buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped);
        if (mapped == nullptr) return;

        std::memcpy(mapped, data, size);
        _context->UnmapBuffer(buffer, Diligent::MAP_WRITE);
    }

    void DiligentRenderer::submitDraws()
    {
        if (!_scenePipeline || _draws.empty()) return;

        const auto *light = findDirectionalLight();
        const auto lightColor = light != nullptr ? ColorNormalize(light->color) : Vector4{};
        const auto ambient = ColorNormalize(_ambientColor);

        const SceneFrameConstants frame{
            .viewProjection = MatrixToFloatV(_viewProjection),
            .cameraPosition = {_cameraPosition.x, _cameraPosition.y, _cameraPosition.z, 1.0f},
            .lightDirection = light != nullptr
                                  ? Vector4{light->direction.x, light->direction.y, light->direction.z, 0.0f}
                                  : Vector4{0.0f, -1.0f, 0.0f, 0.0f},
            .lightColor = {lightColor.x, lightColor.y, lightColor.z, light != nullptr ? light->intensity : 0.0f},
            .ambientColor = {ambient.x, ambient.y, ambient.z, _ambientEnergy},
            .outputEncoding = {_outputEncoding, 0.0f, 0.0f, 0.0f}
        };
        uploadConstants(_frameConstants, &frame, sizeof(frame));

        _context->SetPipelineState(_scenePipeline);

        for (const auto &[mesh, material, model]: _draws)
        {
            const auto *slot = _meshes.get(mesh);
            if (slot == nullptr) continue;

            const SceneDrawConstants draw{
                .model = MatrixToFloatV(model),
                // Inverse transpose, so a non-uniform scale tilts the surface without taking
                // its normals off it.
                .normalMatrix = MatrixToFloatV(MatrixTranspose(MatrixInvert(model)))
            };
            uploadConstants(_drawConstants, &draw, sizeof(draw));
            bindMaterial(material);

            Diligent::IBuffer *vertices = slot->vertices;
            constexpr Diligent::Uint64 vertexOffset = 0;
            _context->SetVertexBuffers(0, 1, &vertices, &vertexOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                       Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
            _context->SetIndexBuffer(slot->indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            // After the draw constants were remapped and the material rebound, so the draw
            // reads this iteration's values and not the ones the previous one left bound.
            _context->CommitShaderResources(_sceneResources, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawIndexedAttribs drawAttribs;
            drawAttribs.IndexType = Diligent::VT_UINT32;
            drawAttribs.NumIndices = slot->indexCount;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->DrawIndexed(drawAttribs);
        }
    }

    void DiligentRenderer::yieldToRaylib()
    {
        _context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->InvalidateState();
        _context->Flush();

        // rlgl tracks the GL state it expects in software and only issues the calls it thinks
        // are needed, so every piece of state the scene pipeline sets differently has to be put
        // back by hand. Culling and depth writes already match rlgl's own defaults; blending
        // and the depth test do not.
        rlEnableColorBlend();
        rlDisableDepthTest();

        // Diligent binds a sampler object per texture unit; rlgl uses none and relies on each
        // texture's own parameters. A sampler left bound overrides those, and its mipmapped
        // minification filter makes raylib's single-level textures incomplete - which samples
        // as opaque black, taking the whole editor UI with it.
        for (Diligent::Uint32 unit = 0; unit < MATERIAL_TEXTURE_COUNT; ++unit) glBindSampler(unit, 0);

        // Diligent enables sRGB framebuffer conversion once at device creation and leaves it
        // on. It is a no-op for its own non-sRGB targets, but raylib's colours are already
        // encoded, so leaving it on would gamma them a second time on the way to the window.
        glDisable(GL_FRAMEBUFFER_SRGB);
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

    void DiligentRenderer::drawSceneTexture(const Rectangle destination)
    {
        if (_overlay.id == 0) return;

        const auto width = static_cast<float>(_overlay.texture.width);
        const auto height = static_cast<float>(_overlay.texture.height);
        DrawTexturePro(_overlay.texture, Rectangle{0, 0, width, -height}, destination, Vector2{0, 0}, 0, WHITE);
    }

    // --- lights ---

    LightHandle DiligentRenderer::createLight(const LightType type)
    {
        return _lights.add(LightState{.type = type});
    }

    void DiligentRenderer::destroyLight(const LightHandle handle)
    {
        _lights.remove(handle);
    }

    bool DiligentRenderer::isLightValid(const LightHandle handle) const
    {
        return _lights.get(handle) != nullptr;
    }

    void DiligentRenderer::updateLight(const LightHandle handle, const LightState &state)
    {
        // Nothing is applied here: a light only exists as constants the scene pass reads, so
        // there is no GPU state to diff the incoming values against.
        if (auto *light = _lights.get(handle)) *light = state;
    }

    const LightState *DiligentRenderer::findDirectionalLight()
    {
        const LightState *found = nullptr;
        _lights.forEachAlive([&found](const LightState &light)
        {
            if (found != nullptr || !light.active || light.type != LightType::Directional) return;
            found = &light;
        });

        return found;
    }

    // --- textures ---

    Diligent::SamplerDesc DiligentRenderer::toNative(const TextureFilterMode filter, const TextureWrapMode wrap)
    {
        Diligent::SamplerDesc desc;
        desc.AddressU = desc.AddressV = desc.AddressW = toNative(wrap);

        switch (filter)
        {
            case TextureFilterMode::Point:
                desc.MinFilter = desc.MagFilter = desc.MipFilter = Diligent::FILTER_TYPE_POINT;
                break;
            case TextureFilterMode::Bilinear:
                desc.MinFilter = desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
                desc.MipFilter = Diligent::FILTER_TYPE_POINT;
                break;
            case TextureFilterMode::Anisotropic4x:
            case TextureFilterMode::Anisotropic8x:
            case TextureFilterMode::Anisotropic16x:
                desc.MinFilter = desc.MagFilter = desc.MipFilter = Diligent::FILTER_TYPE_ANISOTROPIC;
                desc.MaxAnisotropy = filter == TextureFilterMode::Anisotropic4x ? 4 : (filter == TextureFilterMode::Anisotropic8x ? 8 : 16);
                break;
            case TextureFilterMode::Trilinear:
            default:
                desc.MinFilter = desc.MagFilter = desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
                break;
        }

        return desc;
    }

    Diligent::TEXTURE_ADDRESS_MODE DiligentRenderer::toNative(const TextureWrapMode wrap)
    {
        switch (wrap)
        {
            case TextureWrapMode::Clamp: return Diligent::TEXTURE_ADDRESS_CLAMP;
            case TextureWrapMode::MirrorRepeat: return Diligent::TEXTURE_ADDRESS_MIRROR;
            case TextureWrapMode::MirrorClamp: return Diligent::TEXTURE_ADDRESS_MIRROR_ONCE;
            case TextureWrapMode::Repeat:
            default: return Diligent::TEXTURE_ADDRESS_WRAP;
        }
    }

    void DiligentRenderer::finalizeTexture(TextureSlot &slot)
    {
        if (slot.uploaded) return;
        if (slot.decodeJob.valid()) slot.decodeJob.get();

        slot.uploaded = true;
        if (!slot.loader) return;

        slot.loader->CreateTexture(_device, &slot.texture);
        restoreRaylibPixelStore();
        // The decoded pixels live in the loader, and the texture now owns its own copy.
        slot.loader.Release();
        if (!slot.texture) return;

        // A combined-sampler pipeline takes the sampler from the view, so this is where a
        // texture's own filter and wrap actually reach the GPU. CreateSampler de-duplicates
        // identical descriptions internally, so there is nothing to cache here.
        Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
        _device->CreateSampler(toNative(slot.desc.filter, slot.desc.wrap), &sampler);
        slot.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sampler);
    }

    TextureHandle DiligentRenderer::createTexture(const TextureDesc &desc)
    {
        if (!_device) return {};

        const auto handle = _textures.add(TextureSlot{.desc = desc});
        auto *slot = _textures.get(handle);

        // Decoding needs no device, so it runs off the render thread; only the upload in
        // finalizeTexture does. Pool slots keep a stable address, so the job captures one.
        slot->decodeJob = WorkerPool::submit([slot]
        {
            Diligent::TextureLoadInfo loadInfo;
            loadInfo.Name = "Scene texture";
            loadInfo.IsSRGB = slot->desc.isColor;
            Diligent::CreateTextureLoaderFromFile(slot->desc.path.c_str(), Diligent::IMAGE_FILE_FORMAT_UNKNOWN, loadInfo, &slot->loader);
        });

        return handle;
    }

    void DiligentRenderer::destroyTexture(const TextureHandle handle)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return;

        if (slot->decodeJob.valid()) slot->decodeJob.get();
        _textures.remove(handle);
    }

    TextureSize DiligentRenderer::getTextureSize(const TextureHandle handle)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return {};

        finalizeTexture(*slot);
        if (!slot->texture) return {};

        const auto &desc = slot->texture->GetDesc();
        return {.width = static_cast<int>(desc.Width), .height = static_cast<int>(desc.Height)};
    }

    // --- materials ---

    void DiligentRenderer::bindMaterial(const MaterialData &material)
    {
        const TextureHandle handles[MATERIAL_TEXTURE_COUNT]{material.albedo, material.normal, material.orm, material.emission};

        for (size_t index = 0; index < MATERIAL_TEXTURE_COUNT; ++index)
        {
            auto &slot = _materialTextures[index];
            if (slot.variable == nullptr) continue;

            Diligent::ITexture *texture = slot.fallback;
            if (auto *entry = _textures.get(handles[index]))
            {
                finalizeTexture(*entry);
                if (entry->texture) texture = entry->texture;
            }

            slot.variable->Set(texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        }
    }

    // --- meshes ---

    MeshHandle DiligentRenderer::createPrimitive(const MeshPrimitiveData &data, const Vector3 forward)
    {
        if (!_device) return {};

        const MeshData geometry = generatePrimitive(data, forward);
        if (geometry.isEmpty()) return {};

        MeshSlot slot;
        slot.indexCount = static_cast<Diligent::Uint32>(geometry.indices.size());

        Diligent::BufferDesc bufferDesc;
        bufferDesc.Usage = Diligent::USAGE_IMMUTABLE;

        bufferDesc.Name = "Mesh vertices";
        bufferDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        bufferDesc.Size = geometry.vertices.size() * sizeof(MeshVertex);
        const Diligent::BufferData vertexData{geometry.vertices.data(), bufferDesc.Size};
        _device->CreateBuffer(bufferDesc, &vertexData, &slot.vertices);

        bufferDesc.Name = "Mesh indices";
        bufferDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
        bufferDesc.Size = geometry.indices.size() * sizeof(uint32_t);
        const Diligent::BufferData indexData{geometry.indices.data(), bufferDesc.Size};
        _device->CreateBuffer(bufferDesc, &indexData, &slot.indices);

        if (!slot.vertices || !slot.indices)
        {
            Logger::LogError("Diligent failed to upload a primitive's geometry");
            return {};
        }

        return _meshes.add(std::move(slot));
    }

    void DiligentRenderer::destroyMesh(const MeshHandle handle)
    {
        // The slot owns its buffers, so clearing it releases them.
        _meshes.remove(handle);
    }

    void DiligentRenderer::drawMesh(const MeshHandle handle, const MaterialData &material, const Vector3 position, const Quaternion rotation, const Vector3 scale)
    {
        if (_meshes.get(handle) == nullptr) return;

        const Matrix model = MatrixMultiply(MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z),
                                                           QuaternionToMatrix(rotation)),
                                            MatrixTranslate(position.x, position.y, position.z));
        _draws.push_back(DrawItem{.mesh = handle, .material = material, .model = model});
    }

    // --- models ---

    ModelHandle DiligentRenderer::loadModel(const std::string &path)
    {
        return {};
    }

    void DiligentRenderer::destroyModel(const ModelHandle handle)
    {
    }

    int DiligentRenderer::getModelMaterialCount(const ModelHandle handle) const
    {
        return 0;
    }

    void DiligentRenderer::setModelMaterial(const ModelHandle handle, const int slot, const MaterialData &material)
    {
    }

    void DiligentRenderer::drawModel(const ModelHandle handle, const Vector3 position, const Quaternion rotation, const Vector3 scale)
    {
    }

    // --- environment ---

    void DiligentRenderer::setEnvironment(const EnvironmentSettings &settings)
    {
        _clearColor = settings.background.color;
        _ambientColor = settings.ambient.color;
        _ambientEnergy = settings.ambient.energy;
    }

    CubemapHandle DiligentRenderer::loadCubemap(const std::string &path)
    {
        return {};
    }

    CubemapHandle DiligentRenderer::createProceduralSky(const int size, const SkyboxProceduralParameters &sky)
    {
        return {};
    }

    void DiligentRenderer::destroyCubemap(const CubemapHandle handle)
    {
    }

    AmbientMapHandle DiligentRenderer::createAmbientMap(const CubemapHandle cubemap)
    {
        return {};
    }

    void DiligentRenderer::destroyAmbientMap(const AmbientMapHandle handle)
    {
    }
} // namespace BreadEngine
