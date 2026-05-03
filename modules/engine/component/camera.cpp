#include "camera.h"

#include "node.h"
#include "raymath.h"
#include "transform.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Camera)
    REGISTER_COMPONENT(Camera)

    Camera::Camera()
    {
        setupCamera();
    }

    Camera::Camera(Node *owner)
    {
        setupCamera();
        _owner = owner;
    }

    Camera::~Camera() = default;

    void Camera::update(float deltaTime)
    {
        const auto &transform = _owner->get<Transform>();
        const auto pos = transform.getPosition();
        _nativeCamera.position = pos;
        _nativeCamera.target = Vector3Add(pos, transform.getForward());
        _nativeCamera.up = transform.getUp();
        _nativeCamera.fovy = _fov;
        _nativeCamera.projection = static_cast<int>(_projection);
    }

    void Camera::setPerspective(const CameraType type)
    {
        _projection = type;
    }

    void Camera::setFov(const float fov)
    {
        _fov = fov;
    }

    void Camera::setupCamera()
    {
        _nativeCamera.target = {0.0f, 0.0f, 0.0f};
        _nativeCamera.up = {0.0f, 1.0f, 0.0f};
        _nativeCamera.fovy = _fov;
        _nativeCamera.projection = static_cast<int>(_projection);
    }
} // BreadEngine
