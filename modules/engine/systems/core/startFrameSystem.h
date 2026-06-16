#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStartFrameSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void startFrameInternal(Node *node, float deltaTime) = 0;
    };

    template<typename Derived>
    class StartFrameSystem : public IStartFrameSystem
    {
    public:
        virtual void startFrame(Node *node, float deltaTime) = 0;

    private:
        void startFrameInternal(Node *node, const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            if constexpr (HasCustomIsValid<Derived>)
            {
                if (!static_cast<Derived *>(this)->isValid(node)) return;
            }
            static_cast<Derived *>(this)->startFrame(node, deltaTime);
        }
    };
} // namespace BreadEngine
