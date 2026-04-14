#include "cameraDirectorSystem.h"
#include "component/cameraDirector.h"

namespace BreadEngine {
    void CameraDirectorSystem::update(const std::vector<Node *> &nodes, const float deltaTime)
    {
        CameraDirector *cameraDirector = nullptr;
        _cameras.clear();
        for (auto *node: nodes)
        {
            if (node->has<CameraDirector>())
            {
                cameraDirector = &node->get<CameraDirector>();
            }
            else if (node->has<Camera>())
            {
                _cameras.emplace_back(&node->get<Camera>());
            }
        }

        if (cameraDirector == nullptr)
        {
            Logger::LogError("CameraDirectorSystem: No camera director found");
            return;
        }

        cameraDirector->_cameras.clear();
        cameraDirector->_cameras.insert(cameraDirector->_cameras.end(), _cameras.begin(), _cameras.end());
    }
} // BreadEngine
