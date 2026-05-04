#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class DisposeSystem : public SystemBase
    {
    public:
        DisposeSystem() = default;

        [[nodiscard]] bool isDispose() const override { return true; }

        [[nodiscard]] bool isValid(Node &node) override;

        virtual void onDispose(const std::vector<Node *> &nodes, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;
        bool _isValidLogicEnabled = true;

        void onDisposeInternal(float deltaTime);
    };
} // BreadEngine