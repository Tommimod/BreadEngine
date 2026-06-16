#pragma once
#include "core/updateSystem.h"

namespace BreadEngine {
    class CameraSystem final : public UpdateSystem<CameraSystem>
    {
    public:
        void update(Node *node, float deltaTime) override;
    };
} // namespace BreadEngine
