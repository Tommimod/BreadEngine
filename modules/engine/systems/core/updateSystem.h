#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class UpdateSystem : SystemBase
    {
    public:
        UpdateSystem() = default;

        [[nodiscard]] bool isUpdate() const override { return true; }

        [[nodiscard]] bool isValid(Node &node) override;

        virtual void update(const std::vector<Node *> &nodes, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;
        bool _isValidLogicEnabled = true;

        void updateInternal(float deltaTime);
    };
} // BreadEngine
