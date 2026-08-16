#pragma once
#include <string>

#include "renderHandles.h"
#include "renderTypes.h"
#include "configs/light/environmentSettings.h"
#include "configs/light/skyboxProceduralParameters.h"

namespace BreadEngine {
    struct MeshPrimitiveData;

    /// Everything the engine is allowed to ask of the GPU.
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        [[nodiscard]] virtual const char *getBackendName() const = 0;

        /// @param sceneWidth,sceneHeight initial internal render resolution.
        virtual void initialize(int sceneWidth, int sceneHeight) = 0;

        virtual void shutdown() = 0;

        // --- frame ---

        /**
         * Creates or resizes the offscreen target the scene is rendered into. Callers that
         * never size a target (the game) get the scene on the backbuffer instead.
         * Reallocates render targets, so call it on resize, not per frame.
         */
        virtual void resizeSceneTarget(int width, int height) = 0;

        /**
         * Selects whether the scene is encoded to gamma space on its way to the target.
         * Pushed once the project's settings are known, which is after initialize().
         */
        virtual void setOutputColorSpace(OutputColorSpace colorSpace) = 0;

        /// Opens the scene pass. Draw calls issued until endScene() belong to it.
        virtual void beginScene(const CameraView &camera) = 0;

        /// Renders everything the scene pass collected.
        virtual void endScene() = 0;

        /**
         * Makes the rendered scene the active raylib draw target, so the editor can draw its
         * 3D overlay on top of it and still be occluded by scene geometry. Only meaningful
         * for backends that share a GL context with raylib.
         */
        virtual void beginSceneOverlay() = 0;

        virtual void endSceneOverlay() = 0;

        /// Composites the offscreen scene into @p destination of the current draw target.
        virtual void drawSceneTexture(Rectangle destination) = 0;

        // --- lights ---

        [[nodiscard]] virtual LightHandle createLight(LightType type) = 0;

        virtual void destroyLight(LightHandle handle) = 0;

        [[nodiscard]] virtual bool isLightValid(LightHandle handle) const = 0;

        /// Changing LightState::type recreates the underlying resource; the handle survives it.
        virtual void updateLight(LightHandle handle, const LightState &state) = 0;

        // --- textures ---

        /// Returns immediately; decoding runs in the background and the upload happens on
        /// first use, so asset loading can run ahead of the frame that needs the texture.
        [[nodiscard]] virtual TextureHandle createTexture(const TextureDesc &desc) = 0;

        virtual void destroyTexture(TextureHandle handle) = 0;

        [[nodiscard]] virtual TextureSize getTextureSize(TextureHandle handle) = 0;

        // --- meshes ---

        /// @param forward orientation for primitives built around a facing direction (quad, poly).
        [[nodiscard]] virtual MeshHandle createPrimitive(const MeshPrimitiveData &data, Vector3 forward) = 0;

        virtual void destroyMesh(MeshHandle handle) = 0;

        virtual void drawMesh(MeshHandle handle, const MaterialData &material, Vector3 position, Quaternion rotation, Vector3 scale) = 0;

        // --- models ---

        [[nodiscard]] virtual ModelHandle loadModel(const std::string &path) = 0;

        virtual void destroyModel(ModelHandle handle) = 0;

        /// How many material slots setModelMaterial accepts for @p handle.
        [[nodiscard]] virtual int getModelMaterialCount(ModelHandle handle) const = 0;

        virtual void setModelMaterial(ModelHandle handle, int slot, const MaterialData &material) = 0;

        virtual void drawModel(ModelHandle handle, Vector3 position, Quaternion rotation, Vector3 scale) = 0;

        // --- environment ---

        virtual void setEnvironment(const EnvironmentSettings &settings) = 0;

        [[nodiscard]] virtual CubemapHandle loadCubemap(const std::string &path) = 0;

        [[nodiscard]] virtual CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky) = 0;

        virtual void destroyCubemap(CubemapHandle handle) = 0;

        /// Precomputes the irradiance and reflection maps used for image-based lighting.
        [[nodiscard]] virtual AmbientMapHandle createAmbientMap(CubemapHandle cubemap) = 0;

        virtual void destroyAmbientMap(AmbientMapHandle handle) = 0;
    };
} // namespace BreadEngine
