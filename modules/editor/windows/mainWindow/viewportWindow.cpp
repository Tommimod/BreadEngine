#include "viewportWindow.h"
#include "editor.h"
#include "raymath.h"
#include "rlgl.h"

namespace BreadEditor {
    std::string ViewportWindow::Id = "Viewport";

    ViewportWindow::ViewportWindow(const std::string &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    ViewportWindow::ViewportWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ViewportWindow::~ViewportWindow() = default;

    void ViewportWindow::draw(const float deltaTime)
    {
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        const auto texture = Editor::getInstance().getViewportRenderTexture();
        if (!texture) return;

        DrawTexturePro(texture->texture,
                       (Rectangle){0, 0, static_cast<float>(texture->texture.width), -static_cast<float>(texture->texture.height)},
                       getViewportSize(),
                       (Vector2){0, 0},
                       0,
                       WHITE);

        if (isMouseOver())
        {
            //draw
        }

        UiWindow::draw(deltaTime);
    }

    void ViewportWindow::update(const float deltaTime)
    {
        updateScrollView(_bounds, _bounds);
        UiWindow::update(deltaTime);
    }

    void ViewportWindow::dispose()
    {
        UiWindow::dispose();
    }

    bool ViewportWindow::isMouseOver() const
    {
        return CheckCollisionPointRec(GetMousePosition(), getViewportSize());
    }

    Vector2 ViewportWindow::getMousePosition() const
    {
        if (!isMouseOver()) return (Vector2){-1.0f, -1.0f};

        const auto texture = Editor::getInstance().getViewportRenderTexture();
        if (!texture) return (Vector2){-1.0f, -1.0f};

        const auto screenMouse = GetMousePosition();
        const auto size = getViewportSize();
        auto localMouse = Vector2Subtract(screenMouse, (Vector2){size.x, size.y});
        const auto scaleX = static_cast<float>(texture->texture.width) / size.width;
        const auto scaleY = static_cast<float>(texture->texture.height) / size.height;
        return (Vector2){localMouse.x * scaleX, localMouse.y * scaleY};
    }

    Ray ViewportWindow::getMouseRay(Vector2 virtualMouse, Camera3D camera, int width, int height)
    {
        Ray ray;
        auto matView = MatrixLookAt(camera.position, camera.target, camera.up);

        auto aspect = static_cast<double>(width) / static_cast<double>(height);
        auto matProj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);

        auto matViewProj = MatrixMultiply(matView, matProj);
        auto matViewProjInv = MatrixInvert(matViewProj);

        auto nx = (2.0f * virtualMouse.x) / static_cast<float>(width) - 1.0f;
        auto ny = 1.0f - (2.0f * virtualMouse.y) / static_cast<float>(height);

        auto MultiplyMatVec = [&](const float x, const float y, const float z, const float w) -> Vector3
        {
            float tx = matViewProjInv.m0 * x + matViewProjInv.m4 * y + matViewProjInv.m8 * z + matViewProjInv.m12 * w;
            float ty = matViewProjInv.m1 * x + matViewProjInv.m5 * y + matViewProjInv.m9 * z + matViewProjInv.m13 * w;
            float tz = matViewProjInv.m2 * x + matViewProjInv.m6 * y + matViewProjInv.m10 * z + matViewProjInv.m14 * w;
            const float tw = matViewProjInv.m3 * x + matViewProjInv.m7 * y + matViewProjInv.m11 * z + matViewProjInv.m15 * w;

            if (tw != 0.0f)
            {
                tx /= tw;
                ty /= tw;
                tz /= tw;
            }

            return (Vector3){tx, ty, tz};
        };

        Vector3 nearPoint = MultiplyMatVec(nx, ny, -1.0f, 1.0f);
        Vector3 farPoint = MultiplyMatVec(nx, ny, 1.0f, 1.0f);

        ray.position = nearPoint;
        ray.direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint));

        return ray;
    }

    Rectangle ViewportWindow::getViewportSize() const
    {
        auto bounds = _bounds;
        bounds.height -= RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
        bounds.y += RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
        return bounds;
    }

    void ViewportWindow::subscribe()
    {
        UiWindow::subscribe();
    }

    void ViewportWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }

    void ViewportWindow::updateScrollView(const Rectangle &targetWidthRect, const Rectangle &targetHeightRect)
    {
        _contentView.x = _bounds.x;
        _contentView.y = _bounds.y;
        _contentView.height = _bounds.height - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH) - GuiGetStyle(DEFAULT, BORDER_WIDTH) - 15;
        _contentView.width = _bounds.width - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH) - GuiGetStyle(DEFAULT, BORDER_WIDTH) - 1;
    }
} // BreadEditor
