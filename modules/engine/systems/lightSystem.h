#pragma once
#include "core/startFrameSystem.h"

namespace BreadEngine {
    /**
     * Pushes each light's state to the renderer. A start-frame system rather than an update one
     * for two reasons: the update phase skips inactive nodes outright, which would leave a
     * deactivated light shining with the last state it pushed - and this way light and geometry
     * are sampled in the same phase, from the same transforms.
     */
    class LightSystem final : public StartFrameSystem<LightSystem>
    {
    public:
        void startFrame(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
