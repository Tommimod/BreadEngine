#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class EndOfFrameSystem : public SystemBase
    {
    public:
        EndOfFrameSystem() = default;

        [[nodiscard]] bool isEndOnFrame() const override { return true; }

        [[nodiscard]] bool isValid(Node &node) override;

        virtual void endOnFrame(const std::vector<Node *> &nodes, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;
        bool _isValidLogicEnabled = true;

        void endOfFrameInternal(float deltaTime);
    };
} // BreadEngine
