#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IEndOfFrameSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void endOfFrameInternal(Node *node, float deltaTime) = 0;
    };

    template<typename Derived>
    class EndOfFrameSystem : public IEndOfFrameSystem
    {
    public:
        virtual void endOnFrame(Node *node, float deltaTime) = 0;

    private:
        void endOfFrameInternal(Node *node, const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            if constexpr (HasCustomIsValid<Derived>)
            {
                if (!static_cast<Derived *>(this)->isValid(node)) return;
            }
            static_cast<Derived *>(this)->endOnFrame(node, deltaTime);
        }
    };
} // namespace BreadEngine
