#pragma once
#include "camera.h"
#include "core/component.h"
#include <vector>

namespace BreadEngine {
    struct CameraDirector final : Component
    {
        [[nodiscard]] Camera &getActiveCamera() const;

        void transitionToCamera(int cameraId);

    private:
        friend class CameraDirectorSystem;

        std::vector<Camera *> _cameras{};
        Camera *_activeCamera = nullptr;

        INSPECTOR_BEGIN(CameraDirector)
            INSPECT_FIELD(_activeCamera)
        INSPECTOR_END()
    };
} // BreadEngine
