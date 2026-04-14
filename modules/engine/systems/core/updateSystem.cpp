#include "updateSystem.h"

namespace BreadEngine {
    bool UpdateSystem::isValid(Node &node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void UpdateSystem::update(const std::vector<Node *> &nodes, float deltaTime)
    {
    }

    void UpdateSystem::updateInternal(const float deltaTime)
    {
        if (_isValidLogicEnabled)
        {
            _sortedNodes.clear();
            for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
            {
                if (!isValid(*node)) continue;
                _sortedNodes.emplace_back(node);
            }

            update(_sortedNodes, deltaTime);
        }
        else update(NodeProvider::getAllNodes(), deltaTime);
    }
} // BreadEngine
