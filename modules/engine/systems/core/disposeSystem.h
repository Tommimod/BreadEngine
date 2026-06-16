#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IDisposeSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void onDisposeInternal(Node *node, float deltaTime) = 0;
    };

    template<typename Derived>
    class DisposeSystem : public IDisposeSystem
    {
    public:
        virtual void onDispose(Node *node, float deltaTime) = 0;

    private:
        void onDisposeInternal(Node *node, const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            if constexpr (HasCustomIsValid<Derived>)
            {
                if (!static_cast<Derived *>(this)->isValid(node)) return;
            }
            static_cast<Derived *>(this)->onDispose(node, deltaTime);
        }
    };
} // namespace BreadEngine
