#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class InitializeSystem : public SystemBase
    {
    public:
        InitializeSystem() = default;
        [[nodiscard]] bool isInitializable() const override { return true; }

        virtual void initialize(const std::vector<Node *> &nodes);

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;

        void initializeInternal();
    };
} // BreadEngine
