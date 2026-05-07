#include "textureAsset.h"

#include <thread>

namespace BreadEngine {
    DEFINE_STATIC_PROPS(TextureAsset)

    Texture2D &TextureAsset::getTexture()
    {
        if (_isLoaded) return _nativeTexture;
        if (_loadThread.joinable()) _loadThread.join();
        if (IsImageValid(_nativeRawImage))
        {
            _nativeTexture = R3D_LoadTextureFromImageEx(_nativeRawImage, _textureWrap, _textureFilter, _withColor);
        }
        else
        {
            _nativeTexture = R3D_LoadTextureEx(getFile()->getFullPath().c_str(), _textureWrap, _textureFilter, _withColor);
        }

        _isLoaded = true;
        return _nativeTexture;
    }

    void TextureAsset::loadToMemory()
    {
        if (_isLoaded || IsImageValid(_nativeRawImage)) return;

        auto path = getFile()->getFullPath().c_str();
        auto func = [this, path]
        {
            _nativeRawImage = LoadImage(path);
        };
        _loadThread = std::thread(func);
        _loadThread.detach();
    }

    void TextureAsset::unload()
    {
        if (_isLoaded) R3D_UnloadTexture(_nativeTexture);
        _isLoaded = false;
    }
} // BreadEngine
