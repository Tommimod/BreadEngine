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
            if (!meshRenderer.isLoaded())
            {
                meshRenderer.loadModel();
            }

            if (meshRenderer.isChangedFromEditor)
            {
                meshRenderer.unload();
                meshRenderer.loadModel();
            }
            if (!meshRenderer.isLoaded()) continue;

            auto &transform = node->get<Transform>();
            if (R3D_IsMeshValid(meshRenderer._nativeMesh))
            {
                R3D_DrawMeshEx(meshRenderer._nativeMesh, meshRenderer._materials[0].getNativeMaterial(), transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
            }
            else
            {
                for (auto i = 0; i < static_cast<int>(meshRenderer._materials.size()); i++)
                {
                    meshRenderer._nativeMeshRenderer.materials[i] = meshRenderer._materials[i].getNativeMaterial();
                }

                R3D_DrawModelEx(meshRenderer._nativeMeshRenderer, transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
            }
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
