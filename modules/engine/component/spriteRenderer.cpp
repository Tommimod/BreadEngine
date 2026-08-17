#include "spriteRenderer.h"

#include "data/primitives/planePrimitiveData.h"
#include "rendering/renderer.h"
#include "rendering/geometry/primitiveGenerator.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(SpriteRenderer)
    REGISTER_COMPONENT(SpriteRenderer)

    /// Pixels per world unit for a sprite quad.
    constexpr float SPRITE_SCALE = .01f;

    SpriteRenderer::SpriteRenderer(Node *owner)
    {
        _owner = owner;
    }

    void SpriteRenderer::onDestroy()
    {
        unload();
    }

    void SpriteRenderer::loadSprite(const Vector3 forward)
    {
        if (_textureAsset == nullptr)
        {
            return;
        }

        const auto size = _textureAsset->getSize();
        PlanePrimitiveData quad;
        quad.asQuad();
        quad.width = static_cast<float>(size.width) * SPRITE_SCALE;
        quad.height = static_cast<float>(size.height) * SPRITE_SCALE;

        _mesh = Renderer::get().createMesh(generatePrimitive(quad, forward));
        _material = Material();
        _material.setAlbedoTexture(_textureAsset);
        _quadTexture = _textureAsset->getTexture();
        _isLoaded = true;
    }

    void SpriteRenderer::unload()
    {
        if (!_isLoaded) return;

        Renderer::get().destroyMesh(_mesh);
        _mesh = {};
        _quadTexture = {};
        _isLoaded = false;
        _material.unload();
    }

    bool SpriteRenderer::isLoaded() const
    {
        return _isLoaded;
    }

    bool SpriteRenderer::isQuadStale()
    {
        return _textureAsset == nullptr || _quadTexture != _textureAsset->getTexture();
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
