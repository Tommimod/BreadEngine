#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStandaloneDisposeSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void standaloneOnDisposeInternal(float deltaTime) = 0;
    };

    template<typename Derived>
    class StandaloneDisposeSystem : public IStandaloneDisposeSystem
    {
    public:
        virtual void onDispose(float deltaTime) = 0;

    private:
        void standaloneOnDisposeInternal(const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            static_cast<Derived *>(this)->onDispose(deltaTime);
        }
    };
} // namespace BreadEngine
