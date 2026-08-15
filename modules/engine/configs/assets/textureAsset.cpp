#include "textureAsset.h"

#include "rendering/renderer.h"

namespace BreadEngine {
    TextureHandle TextureAsset::getTexture()
    {
        loadToMemory();
        return _handle;
    }

    TextureSize TextureAsset::getSize()
    {
        return Renderer::get().getTextureSize(getTexture());
    }

    void TextureAsset::loadToMemory()
    {
        if (_handle.isValid()) return;

        _handle = Renderer::get().createTexture(TextureDesc{
            .path = getAssetPath(),
            .filter = _textureFilter,
            .wrap = _textureWrap,
            .isColor = _withColor
        });
    }

    void TextureAsset::unload()
    {
        Renderer::get().destroyTexture(_handle);
        _handle = {};
    }

    void TextureAsset::setTextureType(const TextureType textureType)
    {
        _textureType = textureType;
    }

    TextureAsset::TextureType TextureAsset::getTextureType() const
    {
        return _textureType;
    }

    DEFINE_STATIC_PROPS(TextureAsset)
} // BreadEngine
