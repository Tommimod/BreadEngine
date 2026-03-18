#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class EndOfFrameSystem : public SystemBase
    {
    public:
        EndOfFrameSystem() = default;
        [[nodiscard]] bool isEndOnFrame() const override { return true; }

        virtual void endOnFrame(const std::vector<Node *> &nodes, float deltaTime);

    private:
        friend class SystemsRegistry;
        std::vector<Node *> _sortedNodes;

        void endOfFrameInternal(float deltaTime);
    };
} // BreadEngine
