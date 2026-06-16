#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IFixedUpdateSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void fixedUpdateInternal(Node *node, float fixedDeltaTime) = 0;
    };

    template<typename Derived>
    class FixedUpdateSystem : public IFixedUpdateSystem
    {
    public:
        virtual void fixedUpdate(Node *node, float fixedDeltaTime) = 0;

    private:
        void fixedUpdateInternal(Node *node, const float fixedDeltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            if constexpr (HasCustomIsValid<Derived>)
            {
                if (!static_cast<Derived *>(this)->isValid(node)) return;
            }
            static_cast<Derived *>(this)->fixedUpdate(node, fixedDeltaTime);
        }
    };
} // namespace BreadEngine
