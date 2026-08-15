#include "diligentRenderer.h"

#include <string>

#include <EngineFactoryOpenGL.h>

#include "logger.h"
#include "rlgl.h"

namespace BreadEngine {
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
        static const std::string message = "Diligent attached to OpenGL " + std::to_string(apiVersion.Major) + "." + std::to_string(apiVersion.Minor);
        Logger::LogInfo(message);

        createSceneTarget(sceneWidth, sceneHeight);
    }

    void DiligentRenderer::shutdown()
    {
        releaseSceneTarget();
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
        colorDesc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
        colorDesc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
        _device->CreateTexture(colorDesc, nullptr, &_sceneColor);

        Diligent::TextureDesc depthDesc = colorDesc;
        depthDesc.Name = "Scene depth";
        depthDesc.Format = Diligent::TEX_FORMAT_D32_FLOAT;
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

    void DiligentRenderer::resizeSceneTarget(const int width, const int height)
    {
        _hasExplicitTarget = true;
        createSceneTarget(width, height);
    }

    void DiligentRenderer::beginScene(const CameraView &camera)
    {
        if (!_hasExplicitTarget) createSceneTarget(GetScreenWidth(), GetScreenHeight());
        if (!_sceneColor) return;

        auto *renderTarget = _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        auto *depthStencil = _sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
        _context->SetRenderTargets(1, &renderTarget, depthStencil, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto clear = ColorNormalize(_clearColor);
        _context->ClearRenderTarget(renderTarget, &clear.x, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(depthStencil, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void DiligentRenderer::endScene()
    {
        if (!_sceneColor) return;

        yieldToRaylib();

        // Without a caller-sized target the scene is the frame, so it goes to the backbuffer
        // raylib is currently drawing into.
        if (!_hasExplicitTarget)
        {
            drawSceneTexture(Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
        }
    }

    void DiligentRenderer::yieldToRaylib()
    {
        _context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->InvalidateState();
        _context->Flush();
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
        return {};
    }

    void DiligentRenderer::destroyMesh(const MeshHandle handle)
    {
    }

    void DiligentRenderer::drawMesh(const MeshHandle handle, const MaterialData &material, const Vector3 position, const Quaternion rotation, const Vector3 scale)
    {
    }

    // --- models ---

    ModelHandle DiligentRenderer::loadModel(const std::string &path)
    {
        return {};
    }

    void DiligentRenderer::destroyModel(const ModelHandle handle)
    {
    }

    int DiligentRenderer::getModelMaterialCount(const std::string &path)
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

    void DiligentRenderer::applyDefaultEnvironment(EnvironmentSettings settings)
    {
        // Nothing to seed: the backend holds no environment state of its own beyond the clear
        // colour, which setEnvironment supplies every frame.
    }

    void DiligentRenderer::setEnvironment(EnvironmentSettings settings)
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
