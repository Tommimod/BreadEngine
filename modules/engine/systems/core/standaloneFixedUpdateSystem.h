#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStandaloneFixedUpdateSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void standaloneFixedUpdateInternal(float fixedDeltaTime) = 0;
    };

    template<typename Derived>
    class StandaloneFixedUpdateSystem : public IStandaloneFixedUpdateSystem
    {
    public:
        virtual void fixedUpdate(float fixedDeltaTime) = 0;

    private:
        void standaloneFixedUpdateInternal(const float fixedDeltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            static_cast<Derived *>(this)->fixedUpdate(fixedDeltaTime);
        }
    };
} // namespace BreadEngine
