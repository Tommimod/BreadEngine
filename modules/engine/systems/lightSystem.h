#pragma once
#include "core/updateSystem.h"

namespace BreadEngine {
    class LightSystem : public UpdateSystem
    {
    public:
        void update(Node *node, float deltaTime) override;
    };
} // BreadEngine
