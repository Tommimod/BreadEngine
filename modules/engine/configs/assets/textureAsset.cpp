#include "textureAsset.h"

#include <thread>

namespace BreadEngine {
    Texture2D const &TextureAsset::getTexture()
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

    R3D_Cubemap const &TextureAsset::getCubemap(const R3D_CubemapLayout layout)
    {
        if (_isLoaded) return _nativeCubemap;
        if (_loadThread.joinable()) _loadThread.join();
        if (IsImageValid(_nativeRawImage))
        {
            _nativeCubemap = R3D_LoadCubemapFromImage(_nativeRawImage, layout);
        }
        else
        {
            _nativeCubemap = R3D_LoadCubemap(getFile()->getFullPath().c_str(), layout);
        }

        _isLoaded = true;
        return _nativeCubemap;
    }

    void TextureAsset::loadToMemory()
    {
        if (_isLoaded || _loadThread.joinable() || IsImageValid(_nativeRawImage)) return;

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
