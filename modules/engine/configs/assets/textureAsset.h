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

        /// The texture as it stands, loading it if it is not resident. Takes no reference:
        /// callers that keep the handle past the call must acquire() instead.
        [[nodiscard]] TextureHandle getTexture();

        [[nodiscard]] TextureSize getSize();

        /// Takes a reference for a holder that is about to store the handle.
        [[nodiscard]] TextureHandle acquire();

        /// Gives back one acquire()'s reference, freeing the texture once none are left.
        void release();

        void loadToMemory() override;

        void setTextureType(TextureType textureType);

        [[nodiscard]] TextureType getTextureType() const;

    private:
        TextureType _textureType = TextureType::Default;
        TextureHandle _handle{};
        /// How many holders acquired this texture. The load the assets registry performs on
        /// every image at startup is uncounted, so the first release down to zero frees a
        /// texture the registry still lists; the next getTexture() loads it again.
        unsigned int _references = 0;
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
