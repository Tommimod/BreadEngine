#pragma once
#include "core/component.h"

namespace BreadEngine {
    struct Camera final : Component
    {
        enum CameraType
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

        Camera3D &getNativeCamera() { return _camera3D; }

    private:
        friend class CameraDirectorSystem;
        friend struct CameraDirector;

        Camera3D _camera3D{};
        CameraType _projection = CAMERA_PERSPECTIVE;
        float _fov = 45.0f;
        int _index = 0;

        void setupCamera();

        INSPECTOR_BEGIN(Camera)
            INSPECT_FIELD(_projection);
            INSPECT_FIELD(_fov);
        INSPECTOR_END()
    };
} // BreadEngine
