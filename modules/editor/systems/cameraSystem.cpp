#include "cameraSystem.h"
#include "cursorSystem.h"
#include "editor.h"
#include "engine.h"
#include "raymath.h"
#include "tracy/Tracy.hpp"

namespace BreadEditor {
    CameraSystem::CameraSystem()
    {
        _yaw = PI;
        _pitch = -PI / 4.0f;
    }

    CameraSystem::~CameraSystem() = default;

    void CameraSystem::update(const float deltaTime)
    {
        ZoneScoped;
        tryPrepare();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && _isPrepared)
        {
            auto &editorCamera = Editor::getInstance().getCamera();
            CursorSystem::setCursor(MOUSE_CURSOR_CROSSHAIR);
            const auto mouseDelta = GetMouseDelta();

            _yaw += mouseDelta.x * _mouseSensitivity;
            _pitch -= mouseDelta.y * _mouseSensitivity;
            _pitch = Clamp(_pitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);

            Vector3 direction;
            direction.x = cosf(_pitch) * cosf(_yaw);
            direction.y = sinf(_pitch);
            direction.z = cosf(_pitch) * sinf(_yaw);
            editorCamera.target = Vector3Add(editorCamera.position, direction);

            const auto forward = Vector3Normalize(Vector3Subtract(editorCamera.target, editorCamera.position));
            const auto right = Vector3CrossProduct(forward, editorCamera.up);
            Vector3 move = {0.0f, 0.0f, 0.0f};

            if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
            if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
            if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
            if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);

            if (Vector3Length(move) > 0.0f)
            {
                move = Vector3Normalize(move);
                move = Vector3Scale(move, _moveSpeed * deltaTime);
                editorCamera.position = Vector3Add(editorCamera.position, move);
                editorCamera.target = Vector3Add(editorCamera.target, move);
            }
        }
    }

    void CameraSystem::tryPrepare()
    {
        ZoneScoped;
        const auto viewportWindow = &Editor::getInstance().mainWindow.getViewportWindow();
        if (!viewportWindow->isActive)
        {
            _isPrepared = false;
            return;
        }

        if (const auto size = viewportWindow->getViewportSize();
            IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Engine::isCollisionPointRec(GetMousePosition(), size))
        {
            _isPrepared = true;
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            _isPrepared = false;
        }
    }
} // BreadEditor
