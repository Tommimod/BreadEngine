#include "meshRendererSystem.h"

#include <algorithm>

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
        // getMaterials() guarantees at least one, but nothing guarantees the list still reaches
        // the slots the imported model named: the inspector can shorten it at any time.
        const auto lastSlot = static_cast<int>(materials.size()) - 1;
        for (const auto &[mesh, materialSlot]: meshRenderer._parts)
        {
            renderer.drawMesh(mesh, materials[std::min(materialSlot, lastSlot)].getHandle(),
                              transform.getPosition(), transform.getRotationQuaternion(), transform.getScale());
        }
    }

    void MeshRendererSystem::onDispose(Node *node, float deltaTime)
    {
        if (!node->has<MeshRenderer>()) return;

        auto &meshRenderer = node->get<MeshRenderer>();
        meshRenderer.unload();
    }
} // BreadEngine
