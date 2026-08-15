#pragma once
#include <string>

#include "renderHandles.h"
#include "renderTypes.h"
#include "configs/light/environmentSettings.h"
#include "configs/light/skyboxProceduralParameters.h"

namespace BreadEngine {
    struct MeshPrimitiveData;

    /**
     * Everything the engine is allowed to ask of the GPU. Exactly one implementation is
     * compiled in, selected by the BREAD_RENDER_BACKEND CMake option.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        [[nodiscard]] virtual const char *getBackendName() const = 0;

        virtual void initialize() = 0;

        virtual void shutdown() = 0;

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

        /// Material count of a model file, without keeping the model loaded.
        [[nodiscard]] virtual int getModelMaterialCount(const std::string &path) = 0;

        virtual void setModelMaterial(ModelHandle handle, int slot, const MaterialData &material) = 0;

        virtual void drawModel(ModelHandle handle, Vector3 position, Quaternion rotation, Vector3 scale) = 0;

        // --- environment ---

        /// Overwrites @p settings with the renderer's own starting values.
        virtual void applyDefaultEnvironment(EnvironmentSettings settings) = 0;

        virtual void setEnvironment(EnvironmentSettings settings) = 0;

        [[nodiscard]] virtual CubemapHandle loadCubemap(const std::string &path) = 0;

        [[nodiscard]] virtual CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky) = 0;

        virtual void destroyCubemap(CubemapHandle handle) = 0;

        /// Precomputes the irradiance and reflection maps used for image-based lighting.
        [[nodiscard]] virtual AmbientMapHandle createAmbientMap(CubemapHandle cubemap) = 0;

        virtual void destroyAmbientMap(AmbientMapHandle handle) = 0;
    };
} // namespace BreadEngine
