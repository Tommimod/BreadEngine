#pragma once
#include <thread>

#include <r3d.h>
#include <r3d_texture.h>

#include "../../IRenderer.h"
#include "../../resourcePool.h"

namespace BreadEngine {
    class R3DRenderer final : public IRenderer
    {
    public:
        [[nodiscard]] const char *getBackendName() const override { return "R3D"; }

        void initialize() override;

        void shutdown() override;

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

        [[nodiscard]] int getModelMaterialCount(const std::string &path) override;

        void setModelMaterial(ModelHandle handle, int slot, const MaterialData &material) override;

        void drawModel(ModelHandle handle, Vector3 position, Quaternion rotation, Vector3 scale) override;

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
            std::jthread decodeJob;
            bool uploaded = false;
        };

        ResourcePool<LightSlot, LightHandle> _lights;
        ResourcePool<TextureSlot, TextureHandle> _textures;
        ResourcePool<R3D_Mesh, MeshHandle> _meshes;
        ResourcePool<R3D_Model, ModelHandle> _models;
        R3D_Material _defaultMaterial{};

        /// Joins the background decode and uploads to the GPU, unless already uploaded.
        static void finalizeTexture(TextureSlot &slot);

        static void releaseTexture(TextureSlot &slot);

        void applyTexture(TextureHandle handle, Texture2D &target);

        R3D_Material buildMaterial(const MaterialData &material);

        static R3D_LightType toNative(LightType type);

        static TextureWrap toNative(TextureWrapMode wrap);

        static TextureFilter toNative(TextureFilterMode filter);
    };
} // namespace BreadEngine
