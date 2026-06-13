#include "disposeSystem.h"

#include "engine.h"

namespace BreadEngine {
    bool DisposeSystem::isValid(const Node *node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void DisposeSystem::onDispose(Node *node, float deltaTime)
    {
    }

    void DisposeSystem::onDisposeInternal(Node *node, const float deltaTime)
    {
        if (onlyRuntime() && !Engine::isRuntime()) return;
        if (_isValidLogicEnabled)
        {
            if (!isValid(node)) return;
            onDispose(node, deltaTime);
        }
        else onDispose(node, deltaTime);
    }
} // BreadEngine
