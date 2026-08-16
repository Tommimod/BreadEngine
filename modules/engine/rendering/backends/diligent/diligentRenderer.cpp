#include "diligentRenderer.h"

#include <cstring>
#include <string>

#include <EngineFactoryOpenGL.h>
#include <Shader.h>

#include "logger.h"
#include "raymath.h"
#include "rlgl.h"

#include "../../geometry/primitiveGenerator.h"

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

    namespace {
        /// Overwrites a whole dynamic constant buffer with one matrix, in the column-major
        /// order rlgl uploads its own in - the order the shaders' cbuffer packing expects.
        void uploadMatrix(Diligent::IDeviceContext &context, Diligent::IBuffer *buffer, const Matrix &matrix)
        {
            void *mapped = nullptr;
            context.MapBuffer(buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped);
            if (mapped == nullptr) return;

            const float16 values = MatrixToFloatV(matrix);
            std::memcpy(mapped, values.v, sizeof(values.v));
            context.UnmapBuffer(buffer, Diligent::MAP_WRITE);
        }
    } // namespace

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
        constantsDesc.Size = sizeof(float16);
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

        constantsDesc.Name = "Frame constants";
        _device->CreateBuffer(constantsDesc, nullptr, &_frameConstants);
        constantsDesc.Name = "Draw constants";
        _device->CreateBuffer(constantsDesc, nullptr, &_drawConstants);

        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shaderSources;
        const std::string shaderDirectory = std::string(GetApplicationDirectory()) + SHADER_DIRECTORY;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(shaderDirectory.c_str(), &shaderSources);

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

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

        _device->CreateGraphicsPipelineState(pipelineInfo, &_scenePipeline);
        if (!_scenePipeline)
        {
            Logger::LogError("Diligent failed to create the scene pipeline state");
            return;
        }

        // Both buffers are bound once, for the pipeline's lifetime: their contents change
        // every frame, but never which buffer the shaders read them from.
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "FrameConstants")->Set(_frameConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "DrawConstants")->Set(_drawConstants);
        _scenePipeline->CreateShaderResourceBinding(&_sceneResources, true);
    }

    void DiligentRenderer::resizeSceneTarget(const int width, const int height)
    {
        _hasExplicitTarget = true;
        createSceneTarget(width, height);
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

    void DiligentRenderer::submitDraws()
    {
        if (!_scenePipeline || _draws.empty()) return;

        uploadMatrix(*_context, _frameConstants, _viewProjection);
        _context->SetPipelineState(_scenePipeline);

        for (const auto &[mesh, model]: _draws)
        {
            const auto *slot = _meshes.get(mesh);
            if (slot == nullptr) continue;

            uploadMatrix(*_context, _drawConstants, model);

            Diligent::IBuffer *vertices = slot->vertices;
            constexpr Diligent::Uint64 vertexOffset = 0;
            _context->SetVertexBuffers(0, 1, &vertices, &vertexOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                       Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
            _context->SetIndexBuffer(slot->indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            // After the draw constants were remapped, so the draw reads this frame's values
            // and not the ones the previous iteration left bound.
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
        return {};
    }

    void DiligentRenderer::destroyLight(const LightHandle handle)
    {
    }

    bool DiligentRenderer::isLightValid(const LightHandle handle) const
    {
        return false;
    }

    void DiligentRenderer::updateLight(const LightHandle handle, const LightState &state)
    {
    }

    // --- textures ---

    TextureHandle DiligentRenderer::createTexture(const TextureDesc &desc)
    {
        return {};
    }

    void DiligentRenderer::destroyTexture(const TextureHandle handle)
    {
    }

    TextureSize DiligentRenderer::getTextureSize(const TextureHandle handle)
    {
        return {};
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
        _draws.push_back(DrawItem{.mesh = handle, .model = model});
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

    void DiligentRenderer::applyDefaultEnvironment(const EnvironmentSettings &settings)
    {
        // Nothing to seed: the backend holds no environment state of its own beyond the clear
        // colour, which setEnvironment supplies every frame.
    }

    void DiligentRenderer::setEnvironment(const EnvironmentSettings &settings)
    {
        _clearColor = settings.background.color;
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
