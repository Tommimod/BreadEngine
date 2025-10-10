#include "transform.h"
#include "raymath.h"

namespace BreadEngine
{
    Transform::Transform(Node *parent) : Component(parent)
    {
        if (!parent)
        {
            TraceLog(LOG_FATAL, "Transform: parent is null");
        }

        isActive = true;
        transformMatrix = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0};
    }

    Transform::~Transform() = default;

    void Transform::setIsActive(bool isActive)
    {
        Component::setIsActive(true);
    }

    std::string Transform::toString() const
    {
        const auto position = "position : x:" + std::to_string(transformMatrix.m0) + " y:" + std::to_string(transformMatrix.m4) + " z:" + std::to_string(transformMatrix.m8);
        const auto rotation = "rotation : x:" + std::to_string(transformMatrix.m1) + " y:" + std::to_string(transformMatrix.m5) + " z:" + std::to_string(transformMatrix.m9) + " w:"
                              +
                              std::to_string(transformMatrix.m13);
        const auto scale = "scale x:" + std::to_string(transformMatrix.m2) + " y:" + std::to_string(transformMatrix.m6) + " z:" + std::to_string(transformMatrix.m10);
        const auto result = "\n" + position + "\n" + rotation + "\n" + scale;
        return Component::toString().append(result);
    }

    Vector3 Transform::getPosition() const
    {
        return Vector3{transformMatrix.m0, transformMatrix.m4, transformMatrix.m8};
    }

    void Transform::setPosition(const Vector3 &position)
    {
        transformMatrix.m0 = position.x;
        transformMatrix.m4 = position.y;
        transformMatrix.m8 = position.z;
    }

    Vector3 Transform::getRotationVector() const
    {
        return QuaternionToEuler(getRotationQuaternion());
    }

    Quaternion Transform::getRotationQuaternion() const
    {
        return Quaternion{transformMatrix.m1, transformMatrix.m5, transformMatrix.m9, transformMatrix.m13};
    }

    void Transform::setRotation(const Vector3 &rotation)
    {
        setRotation(QuaternionFromEuler(rotation.z, rotation.y, rotation.x));
    }

    void Transform::setRotation(const Quaternion &rotation)
    {
        transformMatrix.m1 = rotation.x;
        transformMatrix.m5 = rotation.y;
        transformMatrix.m9 = rotation.z;
        transformMatrix.m13 = rotation.w;
    }

    Vector3 Transform::getScale() const
    {
        return Vector3{transformMatrix.m2, transformMatrix.m6, transformMatrix.m10};
    }

    void Transform::setScale(const Vector3 &scale)
    {
        transformMatrix.m2 = scale.x;
        transformMatrix.m6 = scale.y;
        transformMatrix.m10 = scale.z;
    }

    Matrix Transform::getTransformMatrix() const
    {
        return transformMatrix;
    }

    void Transform::setTransformMatrix(const Matrix &nextTransformMatrix)
    {
        this->transformMatrix = nextTransformMatrix;
    }
} // namespace BreadEngine
