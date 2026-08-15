#include "material.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Material)

    void Material::unload()
    {
        if (_albedoTexture) _albedoTexture->unload();
        if (_normalTexture) _normalTexture->unload();
        if (_omrTexture) _omrTexture->unload();
        if (_emissionTexture) _emissionTexture->unload();

        _albedoTexture = nullptr;
        _normalTexture = nullptr;
        _omrTexture = nullptr;
        _emissionTexture = nullptr;
        _data = {};
    }

    const MaterialData &Material::getData()
    {
        _data.albedo = _albedoTexture ? _albedoTexture->getTexture() : TextureHandle{};
        _data.normal = _normalTexture ? _normalTexture->getTexture() : TextureHandle{};
        _data.orm = _omrTexture ? _omrTexture->getTexture() : TextureHandle{};
        _data.emission = _emissionTexture ? _emissionTexture->getTexture() : TextureHandle{};
        return _data;
    }
} // BreadEngine
