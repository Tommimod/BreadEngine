#pragma once
#include <future>

#include <r3d.h>
#include <r3d_texture.h>

#include "../../IRenderer.h"
#include "../../resourcePool.h"

namespace BreadEngine {
    class R3DRenderer final : public IRenderer
    {
    public:
        [[nodiscard]] const char *getBackendName() const override { return "R3D"; }

        void initialize(int sceneWidth, int sceneHeight) override;

        void shutdown() override;

        void resizeSceneTarget(int width, int height) override;

        void setOutputColorSpace(OutputColorSpace colorSpace) override;

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
        struct LightSlot
        {
            R3D_Light native = -1;
            LightState applied{};
            /// False until the first update, which then writes every property rather than
            /// diffing against a cache that has never matched the light.
            bool hasApplied = false;
        };

        struct TextureSlot
        {
            TextureDesc desc;
            Texture2D native{};
            Image decoded{};
            /// The job writes into this slot, so every path that frees or recycles the slot
            /// has to wait on it first - dropping the future does not wait on its own.
            std::future<void> decodeJob;
            bool uploaded = false;
        };

        ResourcePool<LightSlot, LightHandle> _lights;
        ResourcePool<TextureSlot, TextureHandle> _textures;
        ResourcePool<R3D_Mesh, MeshHandle> _meshes;
        ResourcePool<R3D_Model, ModelHandle> _models;
        ResourcePool<R3D_Cubemap, CubemapHandle> _cubemaps;
        ResourcePool<R3D_AmbientMap, AmbientMapHandle> _ambientMaps;
        R3D_Material _defaultMaterial{};
        /// Zero-id while the scene renders straight to the backbuffer.
        RenderTexture2D _sceneTarget{};

        /// Joins the background decode and uploads to the GPU, unless already uploaded.
        static void finalizeTexture(TextureSlot &slot);

        static void releaseTexture(TextureSlot &slot);

        void applyTexture(TextureHandle handle, Texture2D &target);

        R3D_Material buildMaterial(const MaterialData &material);

        static Camera3D toNative(const CameraView &camera);

        static R3D_LightType toNative(LightType type);

        static R3D_Bloom toNative(BloomMode mode);

        static R3D_Fog toNative(FogMode mode);

        static R3D_DoF toNative(DepthOfFieldMode mode);

        static R3D_Tonemap toNative(TonemapMode mode);

        static BloomMode fromNative(R3D_Bloom mode);

        static FogMode fromNative(R3D_Fog mode);

        static DepthOfFieldMode fromNative(R3D_DoF mode);

        static TonemapMode fromNative(R3D_Tonemap mode);

        static TextureWrap toNative(TextureWrapMode wrap);

        static TextureFilter toNative(TextureFilterMode filter);
    };
} // namespace BreadEngine
