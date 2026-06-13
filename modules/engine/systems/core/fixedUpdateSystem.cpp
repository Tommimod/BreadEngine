#include "fixedUpdateSystem.h"

#include "engine.h"

namespace BreadEngine {
    bool FixedUpdateSystem::isValid(const Node *node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void FixedUpdateSystem::fixedUpdate(Node *node, float fixedDeltaTime)
    {
    }

    void FixedUpdateSystem::fixedUpdateInternal(Node *node, const float fixedDeltaTime)
    {
        if (onlyRuntime() && !Engine::isRuntime()) return;
        if (_isValidLogicEnabled)
        {
            if (!isValid(node)) fixedUpdate(node, fixedDeltaTime);
        }
        else fixedUpdate(node, fixedDeltaTime);
    }
} // BreadEngine
