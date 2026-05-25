#pragma once
#include "node.h"
#include "systemBase.h"

namespace BreadEngine {
    class EndOfFrameSystem : public SystemBase
    {
    public:
        EndOfFrameSystem() = default;

        [[nodiscard]] bool isEndOnFrame() const override { return true; }

        [[nodiscard]] bool isValid(const Node *node) override;

        virtual void endOnFrame(Node *node, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        bool _isValidLogicEnabled = true;

        void endOfFrameInternal(Node *node, float deltaTime);
    };
} // BreadEngine
