#include "initializeSystem.h"

namespace BreadEngine {
    bool InitializeSystem::isValid(const Node *node)
    {
        _isValidLogicEnabled = false;
        return SystemBase::isValid(node);
    }

    void InitializeSystem::initialize(Node *node)
    {
    }

    void InitializeSystem::initializeInternal(Node *node)
    {
        if (_isValidLogicEnabled)
        {
            if (!isValid(node)) return;
            initialize(node);
        }
        else initialize(node);
    }
} // BreadEngine
