#pragma once
#include "core/updateSystem.h"

namespace BreadEngine {
    class LerpPositionSystem : public UpdateSystem
    {
    public:
        LerpPositionSystem() = default;

        void update(Node *node, float deltaTime) override;

    protected:
        bool onlyRuntime() const override { return true; }
    };
} // BreadEngine
