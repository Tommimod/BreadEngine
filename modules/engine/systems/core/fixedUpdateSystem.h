#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class FixedUpdateSystem : public SystemBase
    {
    public:
        FixedUpdateSystem() = default;

        [[nodiscard]] bool isFixedUpdate() const override { return true; }

        [[nodiscard]] bool isValid(Node &node) override;

        virtual void fixedUpdate(const std::vector<Node *> &nodes, float fixedDeltaTime) = 0;

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;
        bool _isValidLogicEnabled = true;

        void fixedUpdateInternal(float fixedDeltaTime);
    };
} // BreadEngine