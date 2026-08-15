#include "cameraSystem.h"

#include "raymath.h"
#include "transform.h"
#include "component/camera.h"

namespace BreadEngine {
    void CameraSystem::update(Node *node, float deltaTime)
    {
        if (!node->has<Camera>()) return;

        auto &camera = node->get<Camera>();
        const auto &transform = node->get<Transform>();
        const auto pos = transform.getPosition();
        camera._nativeCamera.position = pos;
        camera._nativeCamera.target = Vector3Add(pos, transform.getForward());
        camera._nativeCamera.up = transform.getUp();
        camera._nativeCamera.fovy = camera._fov;
        camera._nativeCamera.projection = toRaylibProjection(camera._projection);
    }
} // BreadEngine
