#include "disposeSystem.h"

namespace BreadEngine {
    bool DisposeSystem::isValid(Node &node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void DisposeSystem::onDispose(const std::vector<Node *> &nodes, float deltaTime)
    {
    }

    void DisposeSystem::onDisposeInternal(float deltaTime)
    {
        if (_isValidLogicEnabled)
        {
            _sortedNodes.clear();
            for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
            {
                if (!isValid(*node)) continue;
                _sortedNodes.emplace_back(node);
            }

            onDispose(_sortedNodes, deltaTime);
        }
        else onDispose(NodeProvider::getAllNodes(), deltaTime);
    }
} // BreadEngine
