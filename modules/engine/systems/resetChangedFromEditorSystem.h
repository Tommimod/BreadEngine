#pragma once
#include "core/endFrameSystem.h"

namespace BreadEngine {
    class ResetChangedFromEditorSystem final : public EndOfFrameSystem<ResetChangedFromEditorSystem>
    {
    public:
        void endOnFrame(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
