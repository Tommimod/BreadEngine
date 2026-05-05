#pragma once
#include "inspectorObject.h"
#include "r3d_material.h"
#include "configs/textureAsset.h"

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

        void setAlbedoTexture(TextureAsset *texture) { _albedoTexture = texture; }
        void setNormalTexture(TextureAsset *texture) { _normalTexture = texture; }
        void setOmrTexture(TextureAsset *texture) { _omrTexture = texture; }

    private:
        std::string _shaderPath;
        R3D_Material _nativeMaterial = R3D_GetDefaultMaterial();
        TextureAsset *_albedoTexture = nullptr;
        TextureAsset *_normalTexture = nullptr;
        TextureAsset *_omrTexture = nullptr;

        INSPECTOR_BEGIN(Material)
            INSPECT_FIELD(_shaderPath);
            INSPECT_FIELD(_albedoTexture);
            INSPECT_FIELD(_normalTexture);
            INSPECT_FIELD(_omrTexture);
        INSPECTOR_END()
    };
} // BreadEngine
