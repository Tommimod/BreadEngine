#pragma once
#include "inspectorObject.h"
#include "r3d_material.h"
#include "configs/textureAsset.h"

namespace BreadEngine {
    struct Material : InspectorStruct
    {
        constexpr static Texture2D whiteTex {.id = 0, .width = 1, .height = 1, .mipmaps = 1, .format = PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA};

        Material() = default;

        void setShaderPath(const std::string &path) { _shaderPath = path; }

        R3D_Material const &getNativeMaterial()
        {
            getAlbedoTexture();
            getNormalTexture();
            getMetallicRoughnessTexture();
            return _nativeMaterial;
        }

        Texture2D const &getAlbedoTexture()
        {
            if (_albedoTexture == nullptr) return whiteTex;
            _nativeMaterial.albedo.texture = _albedoTexture->getTexture();
            return _albedoTexture->getTexture();
        }

        Texture2D const &getNormalTexture()
        {
            if (_normalTexture == nullptr) return whiteTex;
            _nativeMaterial.normal.texture = _normalTexture->getTexture();
            return _normalTexture->getTexture();
        }

        Texture2D const &getMetallicRoughnessTexture()
        {
            if (_metallicRoughnessTexture == nullptr) return whiteTex;
            _nativeMaterial.orm.texture = _metallicRoughnessTexture->getTexture();
            return _metallicRoughnessTexture->getTexture();
        }

    private:
        std::string _shaderPath;
        R3D_Material _nativeMaterial = R3D_GetDefaultMaterial();
        TextureAsset *_albedoTexture = nullptr;
        TextureAsset *_normalTexture = nullptr;
        TextureAsset *_metallicRoughnessTexture = nullptr;

        INSPECTOR_BEGIN(Material)
            INSPECT_FIELD(_shaderPath);
            INSPECT_FIELD(_albedoTexture);
            INSPECT_FIELD(_normalTexture);
            INSPECT_FIELD(_metallicRoughnessTexture);
        INSPECTOR_END()
    };
} // BreadEngine
