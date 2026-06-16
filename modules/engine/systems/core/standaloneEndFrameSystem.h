#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStandaloneEndOfFrameSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void standaloneEndOfFrameInternal(float deltaTime) = 0;
    };

    template<typename Derived>
    class StandaloneEndOfFrameSystem : public IStandaloneEndOfFrameSystem
    {
    public:
        virtual void endOnFrame(float deltaTime) = 0;

    private:
        void standaloneEndOfFrameInternal(const float deltaTime) final
        {
            if constexpr (Derived::kOnlyRuntime)
            {
                if (!isEngineRuntime()) return;
            }
            static_cast<Derived *>(this)->endOnFrame(deltaTime);
        }
    };
} // namespace BreadEngine
