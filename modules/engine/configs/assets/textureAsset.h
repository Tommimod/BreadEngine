#pragma once
#include "asset.h"
#include "rendering/renderHandles.h"
#include "rendering/renderTypes.h"

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

        [[nodiscard]] TextureHandle getTexture();

        [[nodiscard]] TextureSize getSize();

        void loadToMemory() override;

        void unload();

        void setTextureType(TextureType textureType);

        [[nodiscard]] TextureType getTextureType() const;

    private:
        TextureType _textureType = TextureType::Default;
        TextureHandle _handle{};
        TextureWrapMode _textureWrap = TextureWrapMode::Repeat;
        TextureFilterMode _textureFilter = TextureFilterMode::Point;
        bool _withColor = true;

        INSPECTOR_BEGIN(TextureAsset)
            INSPECT_FIELD(_textureType);
            INSPECT_FIELD(_withColor);
            INSPECT_FIELD(_textureFilter);
            INSPECT_FIELD(_textureWrap);
        INSPECTOR_END()
    };
} // BreadEngine
