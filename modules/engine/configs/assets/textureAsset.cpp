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

    TextureHandle TextureAsset::acquire()
    {
        loadToMemory();
        // An invalid handle is not counted, so release() cannot be paired with a load that
        // never produced a texture.
        if (_handle.isValid()) ++_references;
        return _handle;
    }

    void TextureAsset::release()
    {
        if (_references == 0) return;
        if (--_references > 0) return;

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
