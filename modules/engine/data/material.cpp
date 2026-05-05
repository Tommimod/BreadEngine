#include "material.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Material)
    constexpr static Texture2D whiteTex{.id = 0, .width = 1, .height = 1, .mipmaps = 1, .format = PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA};

    void Material::unload()
    {
        if (_albedoTexture) _albedoTexture->unload();
        if (_normalTexture) _normalTexture->unload();
        if (_omrTexture) _omrTexture->unload();

        _albedoTexture = nullptr;
        _normalTexture = nullptr;
        _omrTexture = nullptr;
    }

    R3D_Material const &Material::getNativeMaterial()
    {
        getAlbedoTexture();
        getNormalTexture();
        getOmrTexture();
        return _nativeMaterial;
    }

    Texture2D const &Material::getAlbedoTexture()
    {
        if (_albedoTexture == nullptr) return whiteTex;
        _nativeMaterial.albedo.texture = _albedoTexture->getTexture();
        return _albedoTexture->getTexture();
    }

    Texture2D const &Material::getNormalTexture()
    {
        if (_normalTexture == nullptr) return whiteTex;
        _nativeMaterial.normal.texture = _normalTexture->getTexture();
        return _normalTexture->getTexture();
    }

    Texture2D const &Material::getOmrTexture()
    {
        if (_omrTexture == nullptr) return whiteTex;
        _nativeMaterial.orm.texture = _omrTexture->getTexture();
        return _omrTexture->getTexture();
    }
} // BreadEngine
