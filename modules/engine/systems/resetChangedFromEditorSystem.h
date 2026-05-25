#pragma once
#include "core/endFrameSystem.h"

namespace BreadEngine {
    class ResetChangedFromEditorSystem : public EndOfFrameSystem
    {
    public:
        void endOnFrame(Node *node, float deltaTime) override;
    };
} // BreadEngine
