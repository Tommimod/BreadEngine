#include "fixedUpdateSystem.h"

namespace BreadEngine {
    bool FixedUpdateSystem::isValid(Node &node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void FixedUpdateSystem::fixedUpdate(const std::vector<Node *> &nodes, float fixedDeltaTime)
    {
    }

    void FixedUpdateSystem::fixedUpdateInternal(const float fixedDeltaTime)
    {
        if (_isValidLogicEnabled)
        {
            _sortedNodes.clear();
            for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
            {
                if (!isValid(*node)) continue;
                _sortedNodes.emplace_back(node);
            }

            fixedUpdate(_sortedNodes, fixedDeltaTime);
        }
        else fixedUpdate(NodeProvider::getAllNodes(), fixedDeltaTime);
    }
} // BreadEngine
