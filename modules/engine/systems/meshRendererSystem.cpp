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
        // Unload first, so an edit and a first load are one unload/load pair per frame rather
        // than two, and ResetChangedFromEditorSystem stays the only clearer of the flag.
        if (meshRenderer.isChangedFromEditor) meshRenderer.unload();
        if (!meshRenderer.isLoaded()) meshRenderer.load();
        if (!meshRenderer.isLoaded()) return;

        auto &transform = node->get<Transform>();
        auto &renderer = Renderer::get();
        auto &materials = meshRenderer.getMaterials();
        if (meshRenderer._mesh.isValid())
        {
            renderer.drawMesh(meshRenderer._mesh, materials[0].getHandle(), transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
            return;
        }

        if (!meshRenderer._model.isValid()) return;

        for (auto i = 0; i < static_cast<int>(materials.size()); i++)
        {
            renderer.setModelMaterial(meshRenderer._model, i, materials[i].getHandle());
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
