#pragma once
#include "core/endFrameSystem.h"

namespace BreadEngine {
    class ResetChangedFromEditorSystem : public EndOfFrameSystem
    {
    public:
        void endOnFrame(const std::vector<Node *> &nodes, float deltaTime) override;
    };
} // BreadEngine
