#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IStandaloneInitializeSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void standaloneInitializeInternal() = 0;
    };

    template<typename Derived>
    class StandaloneInitializeSystem : public IStandaloneInitializeSystem
    {
    public:
        virtual void initialize() = 0;

    private:
        void standaloneInitializeInternal() final
        {
            static_cast<Derived *>(this)->initialize();
        }
    };
} // namespace BreadEngine
