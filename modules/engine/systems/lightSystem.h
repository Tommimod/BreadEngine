#pragma once
#include "core/initializeSystem.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class LightSystem : public InitializeSystem, public UpdateSystem
    {
    public:
        void initialize(const std::vector<Node *> &nodes) override;

        void update(const std::vector<Node *> &nodes, float deltaTime) override;
    };
} // BreadEngine
