#pragma once
#include "core/component.h"
#include <raylib.h>

namespace BreadEngine {
    struct Transform final : Component
    {
        Transform();

        explicit Transform(Node *owner);

        ~Transform() override;

        void setIsActive(bool isActive) override;

        [[nodiscard]] std::string toString() const override;

        [[nodiscard]] Vector3 getPosition() const;

        void setPosition(const Vector3 &position);

        [[nodiscard]] Vector3 getRotationVector() const;

        [[nodiscard]] Quaternion getRotationQuaternion() const;

        void setRotation(const Vector3 &rotation);

        void setRotation(const Quaternion &rotation);

        [[nodiscard]] Vector3 getScale() const;

        void setScale(const Vector3 &scale);

        [[nodiscard]] Vector3 getLocalPosition() const;

        void setLocalPosition(const Vector3 &localPosition);

        [[nodiscard]] Vector3 getLocalRotationVector() const;

        [[nodiscard]] Quaternion getLocalRotationQuaternion() const;

        void setLocalRotation(const Vector3 &localRotation);

        void setLocalRotation(const Quaternion &localRotation);

        [[nodiscard]] Vector3 getLocalScale() const;

        void setLocalScale(const Vector3 &localScale);

        [[nodiscard]] Vector3 getForward() const;

        [[nodiscard]] Vector3 getRight() const;

        [[nodiscard]] Vector3 getUp() const;

    private:
        Vector3 _localPosition{};
        Quaternion _localRotation{};
        Vector3 _localScale{};

        INSPECTOR_BEGIN(Transform)
            INSPECT_FIELD(_localPosition);
            INSPECT_FIELD(_localRotation);
            INSPECT_FIELD(_localScale);
        INSPECTOR_END()

        [[nodiscard]] Transform *getParentTransform() const;
    };
} // namespace BreadEngine
