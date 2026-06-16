#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStandaloneStartFrameSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void standaloneStartFrameInternal(float deltaTime) = 0;
    };

    template<typename Derived>
    class StandaloneStartFrameSystem : public IStandaloneStartFrameSystem
    {
    public:
        virtual void startFrame(float deltaTime) = 0;

    private:
        void standaloneStartFrameInternal(const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            static_cast<Derived *>(this)->startFrame(deltaTime);
        }
    };
} // namespace BreadEngine
