#pragma once
#include "core/endFrameSystem.h"
#include "core/fixedUpdateSystem.h"
#include "core/initializeSystem.h"
#include "core/startFrameSystem.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class NodeSystem : public InitializeSystem, public UpdateSystem, public FixedUpdateSystem, public StartFrameSystem, public EndOfFrameSystem
    {
    public:
        void initialize(const std::vector<Node *> &nodes) override;

        void update(const std::vector<Node *> &nodes, float deltaTime) override;

        void fixedUpdate(const std::vector<Node *> &nodes, float fixedDeltaTime) override;

        void startFrame(const std::vector<Node *> &nodes, float deltaTime) override;

        void endOnFrame(const std::vector<Node *> &nodes, float deltaTime) override;
    };
} // BreadEngine
