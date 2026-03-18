#include "endFrameSystem.h"

namespace BreadEngine {
    void EndOfFrameSystem::endOnFrame(const std::vector<Node *> &nodes, float deltaTime)
    {
    }

    void EndOfFrameSystem::endOfFrameInternal(const float deltaTime)
    {
        _sortedNodes.clear();
        for (auto &allNodes = NodeProvider::getAllNodes(); auto node: allNodes)
        {
            if (!isValid(*node)) continue;
            _sortedNodes.emplace_back(node);
        }

        endOnFrame(_sortedNodes, deltaTime);
    }
} // BreadEngine
