#include "cameraDirector.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(CameraDirector)
    REGISTER_COMPONENT(CameraDirector)

    CameraDirector::CameraDirector(Node *owner)
    {
        _owner = owner;
    }

    Camera *CameraDirector::getActiveCamera() const
    {
        return _activeCamera;
    }

    void CameraDirector::transitionToCamera(const int cameraId)
    {
        for (const auto camera: _cameras)
        {
            const auto isActiveCamera = camera->_index == cameraId;
            camera->setIsActive(isActiveCamera);
            if (isActiveCamera) _activeCamera = camera;
        }
    }
} // BreadEngine
