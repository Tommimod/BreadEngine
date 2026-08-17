#include "cameraDirector.h"

#include "node.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(CameraDirector)
    REGISTER_COMPONENT(CameraDirector)

    CameraDirector::CameraDirector(Node *owner)
    {
        _owner = owner;
    }

    Camera *CameraDirector::getActiveCamera() const
    {
        if (_activeCamera == nullptr) return nullptr;
        const auto owner = _activeCamera->getOwner();
        if (owner != nullptr && !owner->getIsActive()) return nullptr;

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
