#pragma once
#include "core/disposeSystem.h"
#include "core/startFrameSystem.h"

namespace BreadEngine {
    class SpriteRendererSystem : public StartFrameSystem, public DisposeSystem
    {
    public:
        void startFrame(Node *node, float deltaTime) override;

        void onDispose(Node *node, float deltaTime) override;
    };
} // BreadEngine
