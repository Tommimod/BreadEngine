#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class IInitializeSystem : public virtual SystemBase
    {
        friend class SystemsRegistry;
        virtual void initializeInternal(Node *node) = 0;
    };

    template<typename Derived>
    class InitializeSystem : public IInitializeSystem
    {
    public:
        virtual void initialize(Node *node) = 0;

    private:
        void initializeInternal(Node *node) final
        {
            if constexpr (HasCustomIsValid<Derived>)
            {
                if (!static_cast<Derived *>(this)->isValid(node)) return;
            }
            static_cast<Derived *>(this)->initialize(node);
        }
    };
} // namespace BreadEngine
