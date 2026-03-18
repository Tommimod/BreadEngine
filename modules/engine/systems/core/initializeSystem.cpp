#include "initializeSystem.h"

namespace BreadEngine {
    void InitializeSystem::initialize(const std::vector<Node *> &nodes)
    {
    }

    void InitializeSystem::initializeInternal()
    {
        _sortedNodes.clear();
        for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
        {
            if (!isValid(*node)) continue;
            _sortedNodes.emplace_back(node);
        }

        initialize(_sortedNodes);
    }
} // BreadEngine
