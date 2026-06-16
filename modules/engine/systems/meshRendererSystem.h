#pragma once
#include "core/disposeSystem.h"
#include "core/startFrameSystem.h"

namespace BreadEngine {
    class MeshRendererSystem final : public StartFrameSystem<MeshRendererSystem>, public DisposeSystem<MeshRendererSystem>
    {
    public:
        void startFrame(Node *node, float deltaTime) override;

        void onDispose(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
