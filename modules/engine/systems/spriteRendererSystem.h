#pragma once
#include "core/disposeSystem.h"
#include "core/startFrameSystem.h"

namespace BreadEngine {
    class SpriteRendererSystem : public StartFrameSystem, public DisposeSystem
    {
    public:
        void startFrame(const std::vector<Node *> &nodes, float deltaTime) override;

        void onDispose(const std::vector<Node *> &nodes, float deltaTime) override;
    };
} // BreadEngine
