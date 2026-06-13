#include "endFrameSystem.h"

#include "engine.h"

namespace BreadEngine {
    bool EndOfFrameSystem::isValid(const Node *node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void EndOfFrameSystem::endOnFrame(Node *node, float deltaTime)
    {
    }

    void EndOfFrameSystem::endOfFrameInternal(Node *node, const float deltaTime)
    {
        if (onlyRuntime() && !Engine::isRuntime()) return;
        if (_isValidLogicEnabled)
        {
            if (!isValid(node)) return;
            endOnFrame(node, deltaTime);
        }
        else endOnFrame(node, deltaTime);
    }
} // BreadEngine
