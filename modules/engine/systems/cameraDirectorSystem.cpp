#include "cameraDirectorSystem.h"
#include "component/cameraDirector.h"

namespace BreadEngine {
    void CameraDirectorSystem::update(Node *node, const float deltaTime)
    {
        CameraDirector *cameraDirector = nullptr;
        if (node->has<CameraDirector>())
        {
            cameraDirector = &node->get<CameraDirector>();
        }
        else if (node->has<Camera>())
        {
            _cameras.emplace_back(&node->get<Camera>());
        }

        if (cameraDirector == nullptr)
        {
            return;
        }

        cameraDirector->_cameras.clear();
        cameraDirector->_cameras.insert(cameraDirector->_cameras.end(), _cameras.begin(), _cameras.end());
        _cameras.clear();
    }
} // BreadEngine
