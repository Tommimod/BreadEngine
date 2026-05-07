#pragma once
#include <thread>

#include "asset.h"
#include "r3d_texture.h"

namespace BreadEngine {
    struct TextureAsset : Asset
    {
        TextureAsset() : Asset()
        {
        }

        explicit TextureAsset(const std::string &fileGuid) : Asset(fileGuid)
        {
        }

        ~TextureAsset() override = default;

        Texture2D &getTexture();

        void loadToMemory() override;

        void unload();

    private:
        Image _nativeRawImage{};
        Texture2D _nativeTexture{};
        TextureWrap _textureWrap = TEXTURE_WRAP_REPEAT;
        TextureFilter _textureFilter = TEXTURE_FILTER_POINT;
        std::thread _loadThread{};
        bool _withColor = true;
        bool _isLoaded = false;

        INSPECTOR_BEGIN(TextureAsset)
            INSPECT_FIELD(_withColor);
            INSPECT_FIELD(_textureFilter);
            INSPECT_FIELD(_textureWrap);
        INSPECTOR_END()
    };
} // BreadEngine
