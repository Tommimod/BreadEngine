#include "startFrameSystem.h"

namespace BreadEngine {
    bool StartFrameSystem::isValid(const Node *node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void StartFrameSystem::startFrame(Node *node, float deltaTime)
    {
    }

    void StartFrameSystem::startFrameInternal(Node *node, const float deltaTime)
    {
        if (_isValidLogicEnabled)
        {
            if (!isValid(node)) return;
            startFrame(node, deltaTime);
        }
        else startFrame(node, deltaTime);
    }
} // BreadEngine
