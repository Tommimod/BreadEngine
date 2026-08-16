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
        if (spriteRenderer.isQuadStale())
        {
            spriteRenderer.unload();
            spriteRenderer.loadSprite(transform.getForward());
            // The link can be cleared rather than replaced, leaving nothing to build a quad from.
            if (!spriteRenderer.isLoaded()) return;
        }

        Renderer::get().drawMesh(spriteRenderer._mesh, spriteRenderer._material.getHandle(), transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
    }

    void SpriteRendererSystem::onDispose(Node *node, float deltaTime)
    {
        if (!node->has<SpriteRenderer>()) return;

        auto &spriteRenderer = node->get<SpriteRenderer>();
        spriteRenderer.unload();
    }
} // BreadEngine
