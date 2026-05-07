#include "spriteRenderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(SpriteRenderer)
    REGISTER_COMPONENT(SpriteRenderer)

    SpriteRenderer::SpriteRenderer(Node *owner)
    {
        _owner = owner;
    }

    void SpriteRenderer::loadSprite(const Vector3 forward)
    {
        if (_textureAsset == nullptr)
        {
            return;
        }

        const auto &texture = _textureAsset->getTexture();
        _nativeMeshRenderer = R3D_GenMeshQuad(texture.width * .01f, texture.height * .01f, 1, 1, forward);
        _material = Material();
        _material.setAlbedoTexture(_textureAsset);
        _isLoaded = true;
    }

    void SpriteRenderer::unload()
    {
        if (_isLoaded && R3D_IsMeshValid(_nativeMeshRenderer))
        {
            R3D_UnloadMesh(_nativeMeshRenderer);
            _isLoaded = false;
            _material.unload();
        }
    }

    bool SpriteRenderer::isLoaded() const
    {
        return _isLoaded;
    }

    void SpriteRenderer::setTextureAsset(TextureAsset *textureAsset, const Vector3 forward)
    {
        if (_textureAsset == textureAsset) return;
        _textureAsset = textureAsset;

        if (_isLoaded)
        {
            unload();
            loadSprite(forward);
        }
    }
} // BreadEngine
