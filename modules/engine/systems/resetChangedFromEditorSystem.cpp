#include "resetChangedFromEditorSystem.h"

namespace BreadEngine {
    void ResetChangedFromEditorSystem::endOnFrame(Node *node, float deltaTime)
    {
        const auto allComponents = ComponentsProvider::getAllComponents(node->getId());
        for (const auto &component: allComponents)
        {
            component->isChangedFromEditor = false;
        }
    }
} // BreadEngine
