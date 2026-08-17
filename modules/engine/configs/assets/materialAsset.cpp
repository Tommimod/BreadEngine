#include "materialAsset.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "logger.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MaterialAsset)

    void MaterialAsset::loadToMemory()
    {
        if (_isLoaded) return;

        _isLoaded = true;
        if (isInDeserializationPhase())
        {
            Logger::LogWarning("Material " + getAssetName() + " was loaded during deserialization; its textures will be missing");
        }

        const auto &path = getAssetPath();
        if (!FileExists(path.c_str())) return;
        if (const auto raw = YAML::LoadFile(path); raw.IsMap()) deserialize(raw);
    }

    void MaterialAsset::saveToOwnFile()
    {
        std::ofstream file(getAssetPath());
        if (!file)
        {
            Logger::LogError("Failed to write material " + getAssetPath());
            return;
        }

        file << serialize();
        file.close();
    }

    MaterialAsset::~MaterialAsset()
    {
        if (Renderer::isAlive()) unload();
    }

    void MaterialAsset::unload()
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

    MaterialAsset::TextureLinks MaterialAsset::textureLinks() const
    {
        return {_albedoTexture, _normalTexture, _omrTexture, _emissionTexture};
    }

    bool MaterialAsset::isBuiltFrom(const TextureLinks &links)
    {
        for (size_t slot = 0; slot < TEXTURE_SLOTS; ++slot)
        {
            const auto &[asset, handle] = _acquired[slot];
            if (asset != links[slot]) return false;
            if (asset != nullptr && asset->getTexture() != handle) return false;
        }

        return true;
    }

    MaterialHandle MaterialAsset::getHandle()
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
