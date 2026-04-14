#include "initializeSystem.h"

namespace BreadEngine {
    bool InitializeSystem::isValid(Node &node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void InitializeSystem::initialize(const std::vector<Node *> &nodes)
    {
    }

    void InitializeSystem::initializeInternal()
    {
        if (_isValidLogicEnabled)
        {
            _sortedNodes.clear();
            for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
            {
                if (!isValid(*node)) continue;
                _sortedNodes.emplace_back(node);
            }

            initialize(_sortedNodes);
        }
        else initialize(NodeProvider::getAllNodes());
    }
} // BreadEngine
