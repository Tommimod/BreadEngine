#include "transform.h"
#include "node.h"
#include "raymath.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Transform)
    REGISTER_COMPONENT(Transform)

    Transform::Transform()
    {
        _isActive = true;
        _localPosition = Vector3{0.0f, 0.0f, 0.0f};
        _localRotationQ = QuaternionIdentity();
        _localRotation = QuaternionToEulerDegrees(_localRotationQ);
        _localScale = Vector3{1.0f, 1.0f, 1.0f};
    }

    Transform::Transform(Node *owner)
    {
        if (!owner)
        {
            Logger::LogError("Transform: parent is null");
        }

        setOwner(owner);
        _isActive = true;
        _localPosition = Vector3{0.0f, 0.0f, 0.0f};
        _localRotationQ = QuaternionIdentity();
        _localRotation = QuaternionToEulerDegrees(_localRotationQ);
        _localScale = Vector3{1.0f, 1.0f, 1.0f};
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

        const Vector3 scaledOffset = Vector3{
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

        _localPosition = Vector3{
            (parentScale.x != 0.0f) ? unrotated.x / parentScale.x : 0.0f,
            (parentScale.y != 0.0f) ? unrotated.y / parentScale.y : 0.0f,
            (parentScale.z != 0.0f) ? unrotated.z / parentScale.z : 0.0f
        };
    }

    Vector3 Transform::getRotationVector() const
    {
        return QuaternionToEulerClosest(getRotationQuaternion(), _localRotation);
    }

    Quaternion Transform::getRotationQuaternion() const
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            return _localRotationQ;
        }

        return QuaternionMultiply(parent->getRotationQuaternion(), _localRotationQ);
    }

    void Transform::setRotation(const Vector3 &rotation)
    {
        Quaternion worldQuat = QuaternionFromEuler(
            rotation.z * DEG2RAD,
            rotation.y * DEG2RAD,
            rotation.x * DEG2RAD
        );

        setRotation(worldQuat);
    }

    void Transform::setRotation(const Quaternion &rotation)
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            _localRotationQ = QuaternionNormalize(rotation);
        }
        else
        {
            const Quaternion parentRot = parent->getRotationQuaternion();
            const Quaternion invParent = QuaternionInvert(parentRot);

            _localRotationQ = QuaternionNormalize(
                QuaternionMultiply(invParent, QuaternionNormalize(rotation))
            );
        }

        _localRotation = QuaternionToEulerClosest(_localRotationQ, _localRotation);
    }

    Vector3 Transform::getScale() const
    {
        const Transform *parent = getParentTransform();
        if (!parent)
        {
            return _localScale;
        }

        const Vector3 parentScale = parent->getScale();
        return Vector3{
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
        _localScale = Vector3{
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
        return _localRotation;
    }

    Quaternion Transform::getLocalRotationQuaternion() const { return _localRotationQ; }

    void Transform::setLocalRotation(const Vector3 &localRotation)
    {
        setLocalRotation(QuaternionFromEuler(localRotation.z * DEG2RAD, localRotation.y * DEG2RAD, localRotation.x * DEG2RAD));
    }

    void Transform::setLocalRotation(const Quaternion &localRotation)
    {
        _localRotationQ = QuaternionNormalize(localRotation);
        _localRotation = QuaternionToEulerClosest(_localRotationQ, _localRotation);
    }

    Vector3 Transform::getLocalScale() const { return _localScale; }

    void Transform::setLocalScale(const Vector3 &localScale)
    {
        _localScale = localScale;
    }

    Vector3 Transform::getForward() const
    {
        constexpr auto forward = Vector3{0.0f, 0.0f, 1.0f};
        return Vector3RotateByQuaternion(forward, getLocalRotationQuaternion());
    }

    Vector3 Transform::getRight() const
    {
        constexpr auto right = Vector3{1.0f, 0.0f, 0.0f};
        return Vector3RotateByQuaternion(right, getLocalRotationQuaternion());
    }

    Vector3 Transform::getUp() const
    {
        constexpr auto up = Vector3{0.0f, 1.0f, 0.0f};
        return Vector3RotateByQuaternion(up, getLocalRotationQuaternion());
    }

    Transform *Transform::getParentTransform() const
    {
        const Node *ownerNode = getOwner();
        if (!ownerNode) return nullptr;

        Node *parentNode = ownerNode->getParent();
        if (!parentNode) return nullptr;

        return &parentNode->get<Transform>();
    }

    Vector3 Transform::QuaternionToEulerDegrees(const Quaternion &quaternion)
    {
        Vector3 euler = QuaternionToEuler(quaternion);
        euler.x *= RAD2DEG;
        euler.y *= RAD2DEG;
        euler.z *= RAD2DEG;
        return euler;
    }

    Vector3 Transform::QuaternionToEulerClosest(const Quaternion &quaternion, const Vector3 &reference)
    {
        Vector3 euler = QuaternionToEuler(quaternion);
        euler.x *= RAD2DEG;
        euler.y *= RAD2DEG;
        euler.z *= RAD2DEG;

        auto unwrap = [](const float angle, const float ref) -> float
        {
            float diff = fmodf(angle - ref, 360.0f);
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;
            return ref + diff;
        };

        euler.x = unwrap(euler.x, reference.x);
        euler.y = unwrap(euler.y, reference.y);
        euler.z = unwrap(euler.z, reference.z);

        return euler;
    }

    void Transform::setLocalRotationEditor(const Vector3 &localRotation)
    {
        _localRotation = localRotation;
        _localRotationQ = QuaternionNormalize(QuaternionFromEuler(
            localRotation.z * DEG2RAD,
            localRotation.y * DEG2RAD,
            localRotation.x * DEG2RAD
        ));
    }
} // namespace BreadEngine
