#pragma once
#include "component/camera.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class CameraDirectorSystem final : public UpdateSystem<CameraDirectorSystem>
    {
    public:
        void update(Node *node, float deltaTime) override;

    private:
        std::vector<Camera *> _cameras;
    };
} // namespace BreadEngine
