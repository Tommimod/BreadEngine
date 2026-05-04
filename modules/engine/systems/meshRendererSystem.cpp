#include "meshRendererSystem.h"

#include "meshRenderer.h"
#include "r3d_draw.h"
#include "transform.h"

namespace BreadEngine {
    void MeshRendererSystem::startFrame(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<MeshRenderer>()) continue;

            auto &meshRenderer = node->get<MeshRenderer>();
            if (!meshRenderer._isLoaded)
            {
                meshRenderer.load();
            }

            for (auto &material: meshRenderer._materials)
            {
                meshRenderer._nativeMeshRenderer.materials[0] = material.getNativeMaterial();
            }

            auto &transform = node->get<Transform>();
            R3D_DrawModelEx(meshRenderer._nativeMeshRenderer, transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
        }
    }

    void MeshRendererSystem::onDispose(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<MeshRenderer>()) continue;

            auto &meshRenderer = node->get<MeshRenderer>();
            meshRenderer.unload();
        }
    }
} // BreadEngine
