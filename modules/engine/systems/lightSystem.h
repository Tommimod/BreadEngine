#pragma once
#include "core/updateSystem.h"

namespace BreadEngine {
    class LightSystem : public UpdateSystem
    {
    public:
        void update(const std::vector<Node *> &nodes, float deltaTime) override;
    };
} // BreadEngine
