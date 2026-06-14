#pragma once
#include "core/endFrameSystem.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class LerpPositionSystem : public UpdateSystem, public EndOfFrameSystem
    {
    public:
        LerpPositionSystem() = default;

        void update(Node *node, float deltaTime) override;

        void endOnFrame(Node *node, float deltaTime) override;

    protected:
        bool onlyRuntime() const override { return true; }
    };
} // BreadEngine
