#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IUpdateSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void updateInternal(Node *node, float deltaTime) = 0;
    };

    template<typename Derived>
    class UpdateSystem : public IUpdateSystem
    {
    public:
        virtual void update(Node *node, float deltaTime) = 0;

    private:
        void updateInternal(Node *node, const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            if constexpr (HasCustomIsValid<Derived>)
            {
                if (!static_cast<Derived *>(this)->isValid(node)) return;
            }
            static_cast<Derived *>(this)->update(node, deltaTime);
        }
    };
} // namespace BreadEngine
