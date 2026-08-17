#include "spriteRenderer.h"

#include <type_traits>

#include "data/primitives/planePrimitiveData.h"
#include "rendering/renderer.h"
#include "rendering/geometry/primitiveGenerator.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(SpriteRenderer)
    REGISTER_COMPONENT(SpriteRenderer)

    static_assert(std::is_nothrow_move_constructible_v<SpriteRenderer>);
    static_assert(std::is_nothrow_move_assignable_v<SpriteRenderer>);

    /// Pixels per world unit for a sprite quad.
    constexpr float SPRITE_SCALE = .01f;

    SpriteRenderer::SpriteRenderer(Node *owner)
    {
        _owner = owner;
    }

    SpriteRenderer::SpriteRenderer(const SpriteRenderer &other)
    {
        Component::operator=(other);
        _material = other._material;
        _textureAsset = other._textureAsset;
    }

    SpriteRenderer &SpriteRenderer::operator=(const SpriteRenderer &other)
    {
        if (this == &other) return *this;

        unload();
        Component::operator=(other);
        _material = other._material;
        _textureAsset = other._textureAsset;
        return *this;
    }

    SpriteRenderer::SpriteRenderer(SpriteRenderer &&other) noexcept
    {
        Component::operator=(other);
        _material = std::move(other._material);
        _mesh = other._mesh;
        _textureAsset = other._textureAsset;
        _textureMaterial = other._textureMaterial;
        _acquiredTexture = other._acquiredTexture;
        _quadTexture = other._quadTexture;
        _isLoaded = other._isLoaded;

        other._mesh = {};
        other._textureMaterial = {};
        other._acquiredTexture = nullptr;
        other._quadTexture = {};
        other._isLoaded = false;
    }

    SpriteRenderer &SpriteRenderer::operator=(SpriteRenderer &&other) noexcept
    {
        if (this == &other) return *this;

        unload();
        Component::operator=(other);
        _material = std::move(other._material);
        _mesh = other._mesh;
        _textureAsset = other._textureAsset;
        _textureMaterial = other._textureMaterial;
        _acquiredTexture = other._acquiredTexture;
        _quadTexture = other._quadTexture;
        _isLoaded = other._isLoaded;

        other._mesh = {};
        other._textureMaterial = {};
        other._acquiredTexture = nullptr;
        other._quadTexture = {};
        other._isLoaded = false;
        return *this;
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

        if (Renderer::isAlive())
        {
            auto &renderer = Renderer::get();
            renderer.destroyMesh(_mesh);
            renderer.destroyMaterial(_textureMaterial);
            if (_acquiredTexture != nullptr) _acquiredTexture->release();
        }

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
