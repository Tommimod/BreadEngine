#include "viewportWindow.h"
#include "editor.h"
#include "raymath.h"
#include "rlgl.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string ViewportWindow::Id = "Viewport";

    ViewportWindow::ViewportWindow(const std::string_view &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    ViewportWindow::ViewportWindow(const std::string_view &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ViewportWindow::~ViewportWindow() = default;

    void ViewportWindow::awake()
    {
        UiWindow::awake();
        _content->isActive = false;
        auto horizontalOffset = 0.0f;
        UiButton *gameButton = nullptr;
        UiButton *sceneButton = nullptr;

        sceneButton = &UiPool::buttonPool.get().setup(id + "_sceneButton", getWindowPanel(), "Scene");
        gameButton = &UiPool::buttonPool.get().setup(id + "_gameButton", getWindowPanel(), "Game");
        sceneButton->setSizePercentPermanent({-1, .85f});
        sceneButton->setSize({40, -1});
        sceneButton->setPosition({0, 2});
        sceneButton->setState(STATE_FOCUSED);
        sceneButton->onClick.subscribe([this, gameButton](UiButton *button)
        {
            if (_mode == Scene) return;
            _mode = Scene;
            button->setState(STATE_FOCUSED);
            gameButton->setState(STATE_NORMAL);
        });
        horizontalOffset = sceneButton->getPosition().x + sceneButton->getSize().x;

        gameButton->setSizePercentPermanent({-1, .85f});
        gameButton->setSize({40, -1});
        gameButton->setPosition({horizontalOffset + 1, 2});
        gameButton->onClick.subscribe([this, sceneButton](UiButton *button)
        {
            if (_mode == Game) return;
            _mode = Game;
            button->setState(STATE_FOCUSED);
            sceneButton->setState(STATE_NORMAL);
        });
        horizontalOffset = gameButton->getPosition().x + gameButton->getSize().x;
    }

    void ViewportWindow::draw(const float deltaTime)
    {
        UiWindow::draw(deltaTime);
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
    }

    void ViewportWindow::update(const float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void ViewportWindow::dispose()
    {
        _mousePosition = {};
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
        const auto localMouse = Vector2Subtract(screenMouse, (Vector2){size.x, size.y});
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
        return _content->getBounds();
    }

    void ViewportWindow::subscribe()
    {
        UiWindow::subscribe();
    }

    void ViewportWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }

    void ViewportWindow::initializePanel()
    {
    }

    void ViewportWindow::cleanupPanel()
    {
    }
} // BreadEditor
