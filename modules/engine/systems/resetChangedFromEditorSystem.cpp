#include "resetChangedFromEditorSystem.h"

namespace BreadEngine {
    void ResetChangedFromEditorSystem::endOnFrame(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            auto allComponents = ComponentsProvider::getAllComponents(node->getId());
            for (const auto &component: allComponents)
            {
                component->isChangedFromEditor = false;
            }
        }
    }
} // BreadEngine
