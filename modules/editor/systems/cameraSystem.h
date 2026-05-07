#pragma once

namespace BreadEditor {
    class CameraSystem
    {
    public:
        CameraSystem();
        ~CameraSystem();

        void update(float deltaTime);
    private:
        float _yaw = 0, _pitch = 0;
        float _mouseSensitivity = 0.002f;
        float _moveSpeed = 10.0f;
        bool _isPrepared = false;

        void tryPrepare();
        void syncYawPitchFromCamera();
    };
} // BreadEditor