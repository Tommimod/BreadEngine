#pragma once
#include "core/component.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct Camera final : Component
    {
        enum BackgroundMode : uint8_t
        {
            SOLID_COLOR = 0,
            SKYBOX
        };

        Camera();

        explicit Camera(Node *owner);

        ~Camera() override;

        void setCameraType(ProjectionType type);

        void setFov(float fov);

        Camera3D &getNativeCamera() { return _nativeCamera; }

        [[nodiscard]] BackgroundMode getBackgroundMode() const { return _backgroundMode; }

        [[nodiscard]] Color getBackgroundColor() const { return _backgroundColor; }

    private:
        friend class CameraDirectorSystem;
        friend class CameraSystem;
        friend struct CameraDirector;

        Camera3D _nativeCamera{};
        Color _backgroundColor = WHITE;
        BackgroundMode _backgroundMode = SOLID_COLOR;
        ProjectionType _projection = ProjectionType::Perspective;
        float _fov = 45.0f;
        int _index = 0;

        void setupCamera();

        INSPECTOR_BEGIN(Camera)
            INSPECT_FIELD(_backgroundMode);
            INSPECT_FIELD_COND(_backgroundColor, [](const Camera* cam) {return cam->_backgroundMode == SOLID_COLOR;});
            INSPECT_FIELD(_projection);
            INSPECT_FIELD(_fov);
        INSPECTOR_END()
    };
} // BreadEngine
