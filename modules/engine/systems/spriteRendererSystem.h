#pragma once
#include "core/disposeSystem.h"
#include "core/startFrameSystem.h"

namespace BreadEngine {
    class SpriteRendererSystem final : public StartFrameSystem<SpriteRendererSystem>, public DisposeSystem<SpriteRendererSystem>
    {
    public:
        void startFrame(Node *node, float deltaTime) override;

        void onDispose(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
