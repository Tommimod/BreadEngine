#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class StartFrameSystem : public SystemBase
    {
    public:
        StartFrameSystem() = default;

        [[nodiscard]] bool isStartOnFrame() const override { return true; }

        [[nodiscard]] bool isValid(Node &node) override;

        virtual void startFrame(const std::vector<Node *> &nodes, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;
        bool _isValidLogicEnabled = true;

        void startFrameInternal(float deltaTime);
    };
} // BreadEngine
