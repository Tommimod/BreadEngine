#pragma once
#include "core/component.h"

namespace BreadEngine {
    struct Camera final : Component
    {
        enum CameraType : uint8_t
        {
            CAMERA_PERSPECTIVE = 0,
            CAMERA_ORTHOGRAPHIC
        };

        Camera();

        explicit Camera(Node *owner);

        ~Camera() override;

        void update(float deltaTime) override;

        void setPerspective(CameraType type);

        void setFov(float fov);

    private:
        Camera3D _camera3D{};
        CameraType _projection = CAMERA_PERSPECTIVE;
        float _fov = 45.0f;

        void setupCamera();

        INSPECTOR_BEGIN(Camera)
            INSPECT_FIELD(_projection);
            INSPECT_FIELD(_fov);
        INSPECTOR_END()
    };
} // BreadEngine
