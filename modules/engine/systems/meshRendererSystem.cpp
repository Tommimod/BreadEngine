#include "meshRendererSystem.h"

#include "meshRenderer.h"
#include "transform.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    void MeshRendererSystem::startFrame(Node *node, float deltaTime)
    {
        if (!node->getIsActive()) return;
        if (!node->has<MeshRenderer>()) return;

        auto &meshRenderer = node->get<MeshRenderer>();
        if (!meshRenderer.isLoaded())
        {
            meshRenderer.load();
        }

        if (meshRenderer.isChangedFromEditor)
        {
            meshRenderer.unload();
            meshRenderer.load();
        }
        if (!meshRenderer.isLoaded()) return;

        auto &transform = node->get<Transform>();
        auto &renderer = Renderer::get();
        if (meshRenderer._mesh.isValid())
        {
            renderer.drawMesh(meshRenderer._mesh, meshRenderer._materials[0].getData(), transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
            return;
        }

        if (!meshRenderer._model.isValid()) return;

        for (auto i = 0; i < static_cast<int>(meshRenderer._materials.size()); i++)
        {
            renderer.setModelMaterial(meshRenderer._model, i, meshRenderer._materials[i].getData());
        }

        renderer.drawModel(meshRenderer._model, transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
    }

    void MeshRendererSystem::onDispose(Node *node, float deltaTime)
    {
        if (!node->has<MeshRenderer>()) return;

        auto &meshRenderer = node->get<MeshRenderer>();
        meshRenderer.unload();
    }
} // BreadEngine
