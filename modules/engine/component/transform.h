#pragma once

#include "../component.h"

#include <raylib.h>

namespace BreadEngine
{
    class Transform final : public Component
    {
    public:
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
        Matrix transformMatrix{};
    };
} // namespace BreadEngine
