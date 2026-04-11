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
        _camera3D.position = pos;
        _camera3D.target = Vector3Add(pos, transform.getForward());
        _camera3D.up = transform.getUp();
        _camera3D.fovy = _fov;
        _camera3D.projection = static_cast<int>(_projection);
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
        _camera3D.target = {0.0f, 0.0f, 0.0f};
        _camera3D.up = {0.0f, 1.0f, 0.0f};
        _camera3D.fovy = _fov;
        _camera3D.projection = static_cast<int>(_projection);
    }
} // BreadEngine
