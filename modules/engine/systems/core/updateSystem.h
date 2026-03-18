#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class UpdateSystem : public SystemBase
    {
    public:
        UpdateSystem() = default;
        [[nodiscard]] bool isUpdatable() const override { return true; }

        virtual void update(const std::vector<Node *> &nodes, float deltaTime);

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;

        void updateInternal(float deltaTime);
    };
} // BreadEngine
