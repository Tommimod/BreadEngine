#include "updateSystem.h"

namespace BreadEngine {
    void UpdateSystem::update(const std::vector<Node *> &nodes, float deltaTime)
    {
    }

    void UpdateSystem::updateInternal(float deltaTime)
    {
        _sortedNodes.clear();
        for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
        {
            if (!isValid(*node)) continue;
            _sortedNodes.emplace_back(node);
        }

        update(_sortedNodes, deltaTime);
    }
} // BreadEngine
