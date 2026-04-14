#pragma once

namespace BreadEditor {
    class CameraSystem
    {
    public:
        CameraSystem();
        ~CameraSystem();

        void update(float deltaTime);
    private:
        float _yaw, _pitch;
        float _mouseSensitivity = 0.002f;
        float _moveSpeed = 10.0f;
        bool _isPrepared = false;

        void tryPrepare();
    };
} // BreadEditor