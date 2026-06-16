#pragma once
#include "core/endFrameSystem.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class LerpPositionSystem final : public UpdateSystem<LerpPositionSystem>, public EndOfFrameSystem<LerpPositionSystem>
    {
    public:
        static constexpr bool kOnlyRuntime = true;

        void update(Node *node, float deltaTime) override;

        void endOnFrame(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
