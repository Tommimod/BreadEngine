#include "transform.h"
#include "node.h"
#include "raymath.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Transform)
    REGISTER_COMPONENT(Transform)

    Transform::Transform()
    {
        _isActive = true;
        _localPosition = {0.0f, 0.0f, 0.0f};
        _localRotation = QuaternionIdentity();
        _localScale = {1.0f, 1.0f, 1.0f};
    }

    Transform::Transform(Node *owner)
    {
        if (!owner)
        {
            TraceLog(LOG_FATAL, "Transform: parent is null");
        }

        setOwner(owner);
        _isActive = true;
        _localPosition = {0.0f, 0.0f, 0.0f};
        _localRotation = QuaternionIdentity();
        _localScale = {1.0f, 1.0f, 1.0f};
    }

    Transform::~Transform() = default;

    void Transform::setIsActive(const bool isActive)
    {
        Component::setIsActive(true); // only for transform
    }

    std::string Transform::toString() const
    {
        const Vector3 worldPosV = getPosition();
        const auto worldPos = "world position : x:" + std::to_string(worldPosV.x) + " y:" + std::to_string(worldPosV.y) + " z:" + std::to_string(worldPosV.z);

        const Quaternion worldRotQ = getRotationQuaternion();
        const auto worldRot = "world rotation : x:" + std::to_string(worldRotQ.x) + " y:" + std::to_string(worldRotQ.y) + " z:" + std::to_string(worldRotQ.z) + " w:" + std::to_string(worldRotQ.w);

        const Vector3 worldScaleV = getScale();
        const auto worldScale = "world scale : x:" + std::to_string(worldScaleV.x) + " y:" + std::to_string(worldScaleV.y) + " z:" + std::to_string(worldScaleV.z);

        const Vector3 localPosV = getLocalPosition();
        const auto localPos = "local position : x:" + std::to_string(localPosV.x) + " y:" + std::to_string(localPosV.y) + " z:" + std::to_string(localPosV.z);

        const Quaternion localRotQ = getLocalRotationQuaternion();
        const auto localRot = "local rotation : x:" + std::to_string(localRotQ.x) + " y:" + std::to_string(localRotQ.y) + " z:" + std::to_string(localRotQ.z) + " w:" + std::to_string(localRotQ.w);

        const Vector3 localScaleV = getLocalScale();
        const auto localScale = "local scale : x:" + std::to_string(localScaleV.x) + " y:" + std::to_string(localScaleV.y) + " z:" + std::to_string(localScaleV.z);

        const auto result = "\n" + worldPos + "\n" + worldRot + "\n" + worldScale +
                            "\n--- Local ---\n" + localPos + "\n" + localRot + "\n" + localScale;
        return Component::toString().append(result);
    }

    Vector3 Transform::getPosition() const
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            return _localPosition;
        }

        const Vector3 parentPos = parent->getPosition();
        const Vector3 parentScale = parent->getScale();
        const Quaternion parentRot = parent->getRotationQuaternion();

        const Vector3 scaledOffset = {
            _localPosition.x * parentScale.x,
            _localPosition.y * parentScale.y,
            _localPosition.z * parentScale.z
        };
        const Vector3 rotatedOffset = Vector3RotateByQuaternion(scaledOffset, parentRot);

        return Vector3Add(parentPos, rotatedOffset);
    }

    void Transform::setPosition(const Vector3 &position)
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            _localPosition = position;
            return;
        }

        const Vector3 parentPos = parent->getPosition();
        const Quaternion parentRot = parent->getRotationQuaternion();
        const Vector3 parentScale = parent->getScale();

        const Vector3 delta = Vector3Subtract(position, parentPos);
        const Quaternion invRot = QuaternionInvert(parentRot);
        const Vector3 unrotated = Vector3RotateByQuaternion(delta, invRot);

        _localPosition = {
            (parentScale.x != 0.0f) ? unrotated.x / parentScale.x : 0.0f,
            (parentScale.y != 0.0f) ? unrotated.y / parentScale.y : 0.0f,
            (parentScale.z != 0.0f) ? unrotated.z / parentScale.z : 0.0f
        };
    }

    Vector3 Transform::getRotationVector() const
    {
        return QuaternionToEuler(getRotationQuaternion());
    }

    Quaternion Transform::getRotationQuaternion() const
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            return _localRotation;
        }
        return QuaternionMultiply(parent->getRotationQuaternion(), _localRotation);
    }

    void Transform::setRotation(const Vector3 &rotation)
    {
        setRotation(QuaternionFromEuler(rotation.z, rotation.y, rotation.x));
    }

    void Transform::setRotation(const Quaternion &rotation)
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            _localRotation = rotation;
            return;
        }

        const Quaternion parentRot = parent->getRotationQuaternion();
        const Quaternion invParent = QuaternionInvert(parentRot);
        _localRotation = QuaternionMultiply(invParent, rotation);
    }

    Vector3 Transform::getScale() const
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            return _localScale;
        }

        const Vector3 parentScale = parent->getScale();
        return {
            parentScale.x * _localScale.x,
            parentScale.y * _localScale.y,
            parentScale.z * _localScale.z
        };
    }

    void Transform::setScale(const Vector3 &scale)
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            _localScale = scale;
            return;
        }

        const Vector3 parentScale = parent->getScale();
        _localScale = {
            (parentScale.x != 0.0f) ? scale.x / parentScale.x : 0.0f,
            (parentScale.y != 0.0f) ? scale.y / parentScale.y : 0.0f,
            (parentScale.z != 0.0f) ? scale.z / parentScale.z : 0.0f
        };
    }

    Vector3 Transform::getLocalPosition() const { return _localPosition; }

    void Transform::setLocalPosition(const Vector3 &localPosition)
    {
        _localPosition = localPosition;
    }

    Vector3 Transform::getLocalRotationVector() const
    {
        return QuaternionToEuler(getLocalRotationQuaternion());
    }

    Quaternion Transform::getLocalRotationQuaternion() const { return _localRotation; }

    void Transform::setLocalRotation(const Vector3 &localRotation)
    {
        setLocalRotation(QuaternionFromEuler(localRotation.z, localRotation.y, localRotation.x));
    }

    void Transform::setLocalRotation(const Quaternion &localRotation)
    {
        _localRotation = localRotation;
    }

    Vector3 Transform::getLocalScale() const { return _localScale; }

    void Transform::setLocalScale(const Vector3 &localScale)
    {
        _localScale = localScale;
    }

    Transform *Transform::getParentTransform() const
    {
        const Node *ownerNode = getOwner();
        if (!ownerNode) return nullptr;

        Node *parentNode = ownerNode->getParent();
        if (!parentNode) return nullptr;

        return &parentNode->get<Transform>();
    }
} // namespace BreadEngine
