#include "spriteRendererSystem.h"

#include "r3d_draw.h"
#include "spriteRenderer.h"
#include "transform.h"

namespace BreadEngine {
    void SpriteRendererSystem::startFrame(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<SpriteRenderer>()) continue;

            auto &transform = node->get<Transform>();
            auto &spriteRenderer = node->get<SpriteRenderer>();
            if (!spriteRenderer.isLoaded())
            {
                spriteRenderer.loadSprite(transform.getForward());
            }

            if (!spriteRenderer.isLoaded()) continue;
            if (spriteRenderer._material.getAlbedoTexture().id != spriteRenderer._textureAsset->getTexture().id)
            {
                spriteRenderer.unload();
                spriteRenderer.loadSprite(transform.getForward());
            }

            R3D_DrawMeshEx(spriteRenderer._nativeMeshRenderer, spriteRenderer._material.getNativeMaterial(), transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
        }
    }

    void SpriteRendererSystem::onDispose(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<SpriteRenderer>()) continue;

            auto &spriteRenderer = node->get<SpriteRenderer>();
            spriteRenderer.unload();
        }
    }
} // BreadEngine
