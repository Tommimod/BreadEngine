#include "material.h"

#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Material)

    Material::Material(const Material &other) : InspectorStruct(other)
    {
        copyLinksFrom(other);
    }

    Material &Material::operator=(const Material &other)
    {
        if (this == &other) return *this;

        unload();
        InspectorStruct::operator=(other);
        copyLinksFrom(other);
        return *this;
    }

    Material::~Material()
    {
        // Nodes and assets outlive the renderer during shutdown, where everything it held is
        // already gone and asking for it would throw out of a destructor.
        if (Renderer::isAlive()) unload();
    }

    void Material::copyLinksFrom(const Material &other)
    {
        // Everything a copy carries, which is everything the inspector serializes: a new
        // field of that kind belongs in this list.
        _shaderPath = other._shaderPath;
        _albedoTexture = other._albedoTexture;
        _normalTexture = other._normalTexture;
        _omrTexture = other._omrTexture;
        _emissionTexture = other._emissionTexture;
    }

    void Material::unload()
    {
        if (_handle.isValid()) Renderer::get().destroyMaterial(_handle);
        _handle = {};
        _isResolved = false;

        for (const auto &[asset, handle]: _acquired)
        {
            if (asset != nullptr && handle.isValid()) asset->release();
        }

        _acquired = {};
    }

    Material::TextureLinks Material::textureLinks() const
    {
        return {_albedoTexture, _normalTexture, _omrTexture, _emissionTexture};
    }

    bool Material::isBuiltFrom(const TextureLinks &links)
    {
        for (size_t slot = 0; slot < TEXTURE_SLOTS; ++slot)
        {
            const auto &[asset, handle] = _acquired[slot];
            if (asset != links[slot]) return false;
            if (asset != nullptr && asset->getTexture() != handle) return false;
        }

        return true;
    }

    MaterialHandle Material::getHandle()
    {
        const auto links = textureLinks();
        if (_isResolved && isBuiltFrom(links)) return _handle;

        unload();
        for (size_t slot = 0; slot < TEXTURE_SLOTS; ++slot)
        {
            auto *asset = links[slot];
            _acquired[slot] = {.asset = asset, .handle = asset != nullptr ? asset->acquire() : TextureHandle{}};
        }

        _handle = Renderer::get().createMaterial(MaterialDesc{
            .albedo = _acquired[0].handle,
            .normal = _acquired[1].handle,
            .orm = _acquired[2].handle,
            .emission = _acquired[3].handle
        });
        _isResolved = true;
        return _handle;
    }
} // BreadEngine
