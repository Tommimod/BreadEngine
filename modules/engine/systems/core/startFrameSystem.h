#pragma once
#include "systemBase.h"

namespace BreadEngine {
    class StartFrameSystem : public SystemBase
    {
    public:
        StartFrameSystem() = default;

        [[nodiscard]] bool isStartOnFrame() const override { return true; }

        [[nodiscard]] bool isValid(const Node *node) override;

        virtual void startFrame(Node *node, float deltaTime) = 0;

    private:
        friend class SystemsRegistry;
        bool _isValidLogicEnabled = true;

        void startFrameInternal(Node *node, float deltaTime);
    };
} // BreadEngine
