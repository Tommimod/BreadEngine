#pragma once
#include "component/camera.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class CameraDirectorSystem : public UpdateSystem
    {
    public:
        void update(const std::vector<Node *> &nodes, float deltaTime) override;
    private:
        std::vector<Camera *> _cameras;
    };
} // BreadEngine
