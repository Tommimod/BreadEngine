#include "systemBase.h"
#include "engine.h"

namespace BreadEngine {
    bool SystemBase::isValid(const Node *node) { return true; }

    bool SystemBase::isEngineRuntime() noexcept { return Engine::isRuntime(); }
} // namespace BreadEngine
