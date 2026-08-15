#include "spriteRendererSystem.h"

#include "spriteRenderer.h"
#include "transform.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    void SpriteRendererSystem::startFrame(Node *node, float deltaTime)
    {
        if (!node->getIsActive()) return;
        if (!node->has<SpriteRenderer>()) return;

        const auto &transform = node->get<Transform>();
        auto &spriteRenderer = node->get<SpriteRenderer>();
        if (!spriteRenderer.isLoaded())
        {
            spriteRenderer.loadSprite(transform.getForward());
        }

        if (!spriteRenderer.isLoaded()) return;
        if (spriteRenderer._material.getData().albedo != spriteRenderer._textureAsset->getTexture())
        {
            spriteRenderer.unload();
            spriteRenderer.loadSprite(transform.getForward());
        }

        Renderer::get().drawMesh(spriteRenderer._mesh, spriteRenderer._material.getData(), transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
    }

    void SpriteRendererSystem::onDispose(Node *node, float deltaTime)
    {
        if (!node->has<SpriteRenderer>()) return;

        auto &spriteRenderer = node->get<SpriteRenderer>();
        spriteRenderer.unload();
    }
} // BreadEngine
