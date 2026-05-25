#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class InitializeSystem : public SystemBase
    {
    public:
        InitializeSystem() = default;

        [[nodiscard]] bool isInitialize() const override { return true; }

        [[nodiscard]] bool isValid(const Node *node) override;

        virtual void initialize(Node *node) = 0;

    private:
        friend class SystemsRegistry;
        bool _isValidLogicEnabled = true;

        void initializeInternal(Node *node);
    };
} // BreadEngine
