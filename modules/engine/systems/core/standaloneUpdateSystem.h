#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStandaloneUpdateSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void standaloneUpdateInternal(float deltaTime) = 0;
    };

    template<typename Derived>
    class StandaloneUpdateSystem : public IStandaloneUpdateSystem
    {
    public:
        virtual void update(float deltaTime) = 0;

    private:
        void standaloneUpdateInternal(const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            static_cast<Derived *>(this)->update(deltaTime);
        }
    };
} // namespace BreadEngine
