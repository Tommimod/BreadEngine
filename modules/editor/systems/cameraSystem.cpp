#include "cameraSystem.h"
#include "cursorSystem.h"
#include "editor.h"
#include "engine.h"
#include "raymath.h"

#define PI 3.141592653589793f

namespace BreadEditor {
    CameraSystem::CameraSystem() : _camera(Engine::getInstance().getCamera())
    {
        _yaw = PI;
        _pitch = -PI / 4.0f;
    }

    CameraSystem::~CameraSystem() = default;

    void CameraSystem::update(const float deltaTime)
    {
        tryPrepare();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && _isPrepared)
        {
            CursorSystem::setCursor(MOUSE_CURSOR_CROSSHAIR);
            const auto mouseDelta = GetMouseDelta();

            _yaw += mouseDelta.x * _mouseSensitivity;
            _pitch -= mouseDelta.y * _mouseSensitivity;
            _pitch = Clamp(_pitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);

            Vector3 direction;
            direction.x = cosf(_pitch) * cosf(_yaw);
            direction.y = sinf(_pitch);
            direction.z = cosf(_pitch) * sinf(_yaw);
            _camera.target = Vector3Add(_camera.position, direction);

            const auto forward = Vector3Normalize(Vector3Subtract(_camera.target, _camera.position));
            const auto right = Vector3CrossProduct(forward, _camera.up);
            Vector3 move = {0.0f, 0.0f, 0.0f};

            if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
            if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
            if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
            if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);

            if (Vector3Length(move) > 0.0f)
            {
                move = Vector3Normalize(move);
                move = Vector3Scale(move, _moveSpeed * deltaTime);
                _camera.position = Vector3Add(_camera.position, move);
                _camera.target = Vector3Add(_camera.target, move);
            }
        }
    }

    void CameraSystem::tryPrepare()
    {
        const auto viewportWindow = &Editor::getInstance().mainWindow.getViewportWindow();
        const auto size = viewportWindow->getViewportSize();
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Engine::isCollisionPointRec(GetMousePosition(), size))
        {
            _isPrepared = true;
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            _isPrepared = false;
        }
    }
} // BreadEditor
