#include "updateSystem.h"

#include "engine.h"

namespace BreadEngine {
    bool UpdateSystem::isValid(const Node *node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void UpdateSystem::update(Node *node, float deltaTime)
    {
    }

    void UpdateSystem::updateInternal(Node *node, const float deltaTime)
    {
        if (onlyRuntime() && !Engine::isRuntime()) return;
        if (_isValidLogicEnabled)
        {
            if (!isValid(node)) return;
            update(node, deltaTime);
        }
        else update(node, deltaTime);
    }
} // BreadEngine
