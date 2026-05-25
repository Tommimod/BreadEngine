#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class FixedUpdateSystem : public SystemBase
    {
    public:
        FixedUpdateSystem() = default;

        [[nodiscard]] bool isFixedUpdate() const override { return true; }

        [[nodiscard]] bool isValid(const Node *node) override;

        virtual void fixedUpdate(Node *node, float fixedDeltaTime) = 0;

    private:
        friend class SystemsRegistry;
        bool _isValidLogicEnabled = true;

        void fixedUpdateInternal(Node *node, float fixedDeltaTime);
    };
} // BreadEngine
