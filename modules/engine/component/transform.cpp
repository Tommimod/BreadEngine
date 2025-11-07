#include "transform.h"
#include "raymath.h"

namespace BreadEngine {
    Transform::Transform()
    {
        _isActive = true;
        _transformMatrix = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0};
    }

    Transform::Transform(Node *parent)
    {
        if (!parent)
        {
            TraceLog(LOG_FATAL, "Transform: parent is null");
        }

        setOwner(parent);
        _isActive = true;
        _transformMatrix = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0};
    }

    Transform::~Transform() = default;

    void Transform::setIsActive(bool isActive)
    {
        Component::setIsActive(true);
    }

    std::string Transform::toString() const
    {
        const auto position = "position : x:" + std::to_string(_transformMatrix.m0) + " y:" + std::to_string(_transformMatrix.m4) + " z:" + std::to_string(_transformMatrix.m8);
        const auto rotation = "rotation : x:" + std::to_string(_transformMatrix.m1) + " y:" + std::to_string(_transformMatrix.m5) + " z:" + std::to_string(_transformMatrix.m9) + " w:"
                              +
                              std::to_string(_transformMatrix.m13);
        const auto scale = "scale x:" + std::to_string(_transformMatrix.m2) + " y:" + std::to_string(_transformMatrix.m6) + " z:" + std::to_string(_transformMatrix.m10);
        const auto result = "\n" + position + "\n" + rotation + "\n" + scale;
        return Component::toString().append(result);
    }

    Vector3 Transform::getPosition() const
    {
        return Vector3{_transformMatrix.m0, _transformMatrix.m4, _transformMatrix.m8};
    }

    void Transform::setPosition(const Vector3 &position)
    {
        _transformMatrix.m0 = position.x;
        _transformMatrix.m4 = position.y;
        _transformMatrix.m8 = position.z;
    }

    Vector3 Transform::getRotationVector() const
    {
        return QuaternionToEuler(getRotationQuaternion());
    }

    Quaternion Transform::getRotationQuaternion() const
    {
        return Quaternion{_transformMatrix.m1, _transformMatrix.m5, _transformMatrix.m9, _transformMatrix.m13};
    }

    void Transform::setRotation(const Vector3 &rotation)
    {
        setRotation(QuaternionFromEuler(rotation.z, rotation.y, rotation.x));
    }

    void Transform::setRotation(const Quaternion &rotation)
    {
        _transformMatrix.m1 = rotation.x;
        _transformMatrix.m5 = rotation.y;
        _transformMatrix.m9 = rotation.z;
        _transformMatrix.m13 = rotation.w;
    }

    Vector3 Transform::getScale() const
    {
        return Vector3{_transformMatrix.m2, _transformMatrix.m6, _transformMatrix.m10};
    }

    void Transform::setScale(const Vector3 &scale)
    {
        _transformMatrix.m2 = scale.x;
        _transformMatrix.m6 = scale.y;
        _transformMatrix.m10 = scale.z;
    }

    Matrix Transform::getTransformMatrix() const
    {
        return _transformMatrix;
    }

    void Transform::setTransformMatrix(const Matrix &nextTransformMatrix)
    {
        this->_transformMatrix = nextTransformMatrix;
    }
} // namespace BreadEngine
