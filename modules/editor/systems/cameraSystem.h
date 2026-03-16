#pragma once
#include "raylib.h"

namespace BreadEditor {
    class CameraSystem
    {
    public:
        CameraSystem();
        ~CameraSystem();

        void update(float deltaTime);
    private:
        Camera3D &_camera;
        float _yaw, _pitch;
        float _mouseSensitivity = 0.002f;
        float _moveSpeed = 10.0f;
        bool _isPrepared = false;

        void tryPrepare();
    };
} // BreadEditor