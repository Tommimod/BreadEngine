#pragma once
#include <thread>

#include "asset.h"
#include "r3d_cubemap.h"
#include "r3d_texture.h"

namespace BreadEngine {
    struct TextureAsset : Asset
    {
        enum class TextureType
        {
            Default = 0,
            Cube,
            Ui
        };

        TextureAsset() : Asset()
        {
        }

        explicit TextureAsset(const std::string &fileGuid) : Asset(fileGuid)
        {
        }

        ~TextureAsset() override = default;

        Texture2D const &getTexture();

        R3D_Cubemap const &getCubemap(R3D_CubemapLayout layout = R3D_CUBEMAP_LAYOUT_AUTO_DETECT);

        void loadToMemory() override;

        void unload();

        void setTextureType(TextureType textureType);

        [[nodiscard]] TextureType getTextureType() const;

    private:
        TextureType _textureType = TextureType::Default;
        Image _nativeRawImage{};
        Texture2D _nativeTexture{};
        R3D_Cubemap _nativeCubemap{};
        TextureWrap _textureWrap = TEXTURE_WRAP_REPEAT;
        TextureFilter _textureFilter = TEXTURE_FILTER_POINT;
        std::thread _loadThread{};
        bool _withColor = true;
        bool _isLoaded = false;

        INSPECTOR_BEGIN(TextureAsset)
            INSPECT_FIELD(_textureType);
            INSPECT_FIELD(_withColor);
            INSPECT_FIELD(_textureFilter);
            INSPECT_FIELD(_textureWrap);
        INSPECTOR_END()
    };
} // BreadEngine
