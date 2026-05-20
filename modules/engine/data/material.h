#pragma once
#include "inspectorObject.h"
#include "r3d_material.h"
#include "../configs/assets/textureAsset.h"

namespace BreadEngine {
    struct Material : InspectorStruct
    {
        Material() = default;

        void setShaderPath(const std::string &path) { _shaderPath = path; }

        void unload();

        R3D_Material const &getNativeMaterial();

        Texture2D const &getAlbedoTexture();

        Texture2D const &getNormalTexture();

        Texture2D const &getOmrTexture();

        Texture2D const &getEmissionTexture();

        void setAlbedoTexture(TextureAsset *texture) { _albedoTexture = texture; }
        void setNormalTexture(TextureAsset *texture) { _normalTexture = texture; }
        void setOmrTexture(TextureAsset *texture) { _omrTexture = texture; }
        void setEmissionTexture(TextureAsset *texture) { _emissionTexture = texture; }

    private:
        std::string _shaderPath;
        R3D_Material _nativeMaterial = R3D_GetDefaultMaterial();
        TextureAsset *_albedoTexture = nullptr;
        TextureAsset *_normalTexture = nullptr;
        TextureAsset *_omrTexture = nullptr;
        TextureAsset *_emissionTexture = nullptr;

        Vector2 _uvOffset{};
        Vector2 _uvScale{};
        float _alphaCutoff = 0.01f;

        INSPECTOR_BEGIN(Material)
            INSPECT_FIELD(_shaderPath);
            INSPECT_FIELD(_albedoTexture);
            INSPECT_FIELD(_normalTexture);
            INSPECT_FIELD(_omrTexture);
            INSPECT_FIELD(_emissionTexture);
            INSPECT_FIELD(_uvOffset);
            INSPECT_FIELD(_uvScale);
            INSPECT_FIELD(_alphaCutoff);
        INSPECTOR_END()
    };
} // BreadEngine
