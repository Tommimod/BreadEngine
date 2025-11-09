#pragma once

#include "component.h"
#include <raylib.h>

namespace BreadEngine {
    struct Transform final : Component
    {
        Transform();

        explicit Transform(Node *parent);

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

        [[nodiscard]] Matrix getTransformMatrix() const;

        void setTransformMatrix(const Matrix &nextTransformMatrix);

    private:
        Matrix _transformMatrix{};
        Vector3 _position{};
        Quaternion _rotation{};
        Vector3 _scale{};

        INSPECTOR_BEGIN(Transform)
            INSPECT_FIELD(_position, &LocalClass::_position);
            INSPECT_FIELD(_rotation, &LocalClass::_rotation);
            INSPECT_FIELD(_scale, &LocalClass::_scale);
        INSPECTOR_END()

        void updateProperties();
    };
} // namespace BreadEngine
