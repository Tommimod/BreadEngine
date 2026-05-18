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

        enum BackgroundMode
        {
            SOLID_COLOR = 0,
            SKYBOX
        };

        Camera();

        explicit Camera(Node *owner);

        ~Camera() override;

        void setPerspective(CameraType type);

        void setFov(float fov);

        Camera3D &getNativeCamera() { return _nativeCamera; }

    private:
        friend class CameraDirectorSystem;
        friend class CameraSystem;
        friend struct CameraDirector;

        Camera3D _nativeCamera{};
        Color _backgroundColor = WHITE;
        BackgroundMode _backgroundMode = SOLID_COLOR;
        CameraType _projection = CAMERA_PERSPECTIVE;
        float _fov = 45.0f;
        int _index = 0;

        void setupCamera();

        INSPECTOR_BEGIN(Camera)
            INSPECT_FIELD(_backgroundMode);
            INSPECT_FIELD_COND(_backgroundColor, [](const Camera* cam) {return cam->_backgroundMode == SOLID_COLOR;});
            INSPECT_FIELD_OPT(_projection, Property::READONLY);
            INSPECT_FIELD(_fov);
        INSPECTOR_END()
    };
} // BreadEngine
