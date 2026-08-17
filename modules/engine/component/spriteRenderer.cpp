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

        auto &renderer = Renderer::get();
        _mesh = renderer.createMesh(generatePrimitive(quad, forward));
        _acquiredTexture = _textureAsset;
        _quadTexture = _textureAsset->acquire();
        _textureMaterial = renderer.createMaterial(MaterialDesc{.albedo = _quadTexture});
        _isLoaded = true;
    }

    void SpriteRenderer::unload()
    {
        if (!_isLoaded) return;

        auto &renderer = Renderer::get();
        renderer.destroyMesh(_mesh);
        renderer.destroyMaterial(_textureMaterial);
        if (_acquiredTexture != nullptr) _acquiredTexture->release();

        _mesh = {};
        _textureMaterial = {};
        _acquiredTexture = nullptr;
        _quadTexture = {};
        _isLoaded = false;
    }

    bool SpriteRenderer::isLoaded() const
    {
        return _isLoaded;
    }

    bool SpriteRenderer::isQuadStale()
    {
        return _textureAsset == nullptr || _quadTexture != _textureAsset->getTexture();
    }

    MaterialHandle SpriteRenderer::getMaterialHandle() const
    {
        return _material.isLinked() ? _material.getHandle() : _textureMaterial;
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
