#include "textureAsset.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(TextureAsset)

    Texture2D &TextureAsset::getTexture()
    {
        if (_isLoaded) return _nativeTexture;

        _nativeTexture = R3D_LoadTextureEx(_file->getFullPath().c_str(), _textureWrap, _textureFilter, _withColor);
        _isLoaded = true;
        return _nativeTexture;
    }

    void TextureAsset::unload()
    {
        if (_isLoaded) R3D_UnloadTexture(_nativeTexture);
        _isLoaded = false;
    }
} // BreadEngine
