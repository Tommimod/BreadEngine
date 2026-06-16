#pragma once
#include "core/updateSystem.h"

namespace BreadEngine {
    class LightSystem final : public UpdateSystem<LightSystem>
    {
    public:
        void update(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
