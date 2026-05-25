#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class DisposeSystem : public SystemBase
    {
    public:
        DisposeSystem() = default;

        [[nodiscard]] bool isDispose() const override { return true; }

        [[nodiscard]] bool isValid(const Node *node) override;

        virtual void onDispose(Node *node, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        bool _isValidLogicEnabled = true;

        void onDisposeInternal(Node *node, float deltaTime);
    };
} // BreadEngine
