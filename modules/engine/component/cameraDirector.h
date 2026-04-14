#pragma once
#include "camera.h"
#include "core/component.h"
#include <vector>

namespace BreadEngine {
    struct CameraDirector final : Component
    {
        Camera &getActiveCamera() const;

        void transitionToCamera(int cameraId);

    private:
        friend class CameraDirectorSystem;

        std::vector<Camera *> _cameras{};
        Camera *_activeCamera = nullptr;

        INSPECTOR_BEGIN(CameraDirector)
        INSPECTOR_END()
    };
} // BreadEngine
