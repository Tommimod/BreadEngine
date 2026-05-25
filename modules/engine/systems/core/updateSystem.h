#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class UpdateSystem : SystemBase
    {
    public:
        UpdateSystem() = default;

        [[nodiscard]] bool isUpdate() const override { return true; }

        [[nodiscard]] bool isValid(const Node *node) override;

        virtual void update(Node* node, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        bool _isValidLogicEnabled = true;

        void updateInternal(Node *node, float deltaTime);
    };
} // BreadEngine
