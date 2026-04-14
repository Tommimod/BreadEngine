#include "startFrameSystem.h"

namespace BreadEngine {
    bool StartFrameSystem::isValid(Node &node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void StartFrameSystem::startFrame(const std::vector<Node *> &nodes, float deltaTime)
    {
    }

    void StartFrameSystem::startFrameInternal(const float deltaTime)
    {
        if (_isValidLogicEnabled)
        {
            _sortedNodes.clear();
            for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
            {
                if (!isValid(*node)) continue;
                _sortedNodes.emplace_back(node);
            }

            startFrame(_sortedNodes, deltaTime);
        }
        else startFrame(NodeProvider::getAllNodes(), deltaTime);
    }
} // BreadEngine
