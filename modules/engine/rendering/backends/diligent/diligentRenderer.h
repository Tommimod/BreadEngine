#pragma once
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

#include "../../IRenderer.h"

namespace BreadEngine {
    /**
     * Renders through DiligentEngine, attached to the OpenGL context raylib already created
     * so the scene target is a GL texture rlgl can composite and draw over without a copy.
     */
    class DiligentRenderer final : public IRenderer
    {
    public:
        [[nodiscard]] const char *getBackendName() const override { return "Diligent"; }

        void initialize(int sceneWidth, int sceneHeight) override;

        void shutdown() override;

        void resizeSceneTarget(int width, int height) override;

        void beginScene(const CameraView &camera) override;

        void endScene() override;

        void beginSceneOverlay() override;

        void endSceneOverlay() override;

        void drawSceneTexture(Rectangle destination) override;

        [[nodiscard]] LightHandle createLight(LightType type) override;

        void destroyLight(LightHandle handle) override;

        [[nodiscard]] bool isLightValid(LightHandle handle) const override;

        void updateLight(LightHandle handle, const LightState &state) override;

        [[nodiscard]] TextureHandle createTexture(const TextureDesc &desc) override;

        void destroyTexture(TextureHandle handle) override;

        [[nodiscard]] TextureSize getTextureSize(TextureHandle handle) override;

        [[nodiscard]] MeshHandle createPrimitive(const MeshPrimitiveData &data, Vector3 forward) override;

        void destroyMesh(MeshHandle handle) override;

        void drawMesh(MeshHandle handle, const MaterialData &material, Vector3 position, Quaternion rotation, Vector3 scale) override;

        [[nodiscard]] ModelHandle loadModel(const std::string &path) override;

        void destroyModel(ModelHandle handle) override;

        [[nodiscard]] int getModelMaterialCount(ModelHandle handle) const override;

        void setModelMaterial(ModelHandle handle, int slot, const MaterialData &material) override;

        void drawModel(ModelHandle handle, Vector3 position, Quaternion rotation, Vector3 scale) override;

        void applyDefaultEnvironment(const EnvironmentSettings &settings) override;

        void setEnvironment(const EnvironmentSettings &settings) override;

        [[nodiscard]] CubemapHandle loadCubemap(const std::string &path) override;

        [[nodiscard]] CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky) override;

        void destroyCubemap(CubemapHandle handle) override;

        [[nodiscard]] AmbientMapHandle createAmbientMap(CubemapHandle cubemap) override;

        void destroyAmbientMap(AmbientMapHandle handle) override;

    private:
        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> _device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> _context;
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneColor;
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneDepth;
        /// A raylib framebuffer over the same two GL textures, so an rlgl overlay pass lands
        /// in the attachments Diligent just rendered into instead of a copy of the colour.
        RenderTexture2D _overlay{};
        Color _clearColor = BLACK;
        /// False while the scene target follows the window rather than a caller-chosen size.
        bool _hasExplicitTarget = false;

        void createSceneTarget(int width, int height);

        void releaseSceneTarget();

        /// Undoes the bindings Diligent made behind rlgl's back, so raylib's next draw goes
        /// to the framebuffer it thinks is bound.
        void yieldToRaylib();
    };
} // namespace BreadEngine
