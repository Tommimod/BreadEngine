#pragma once
#include "component/camera.h"
#include "core/updateSystem.h"

namespace BreadEngine {
    class CameraDirectorSystem : public UpdateSystem
    {
    public:
        void update(Node *node, float deltaTime) override;

    private:
        std::vector<Camera *> _cameras;
    };
} // BreadEngine
