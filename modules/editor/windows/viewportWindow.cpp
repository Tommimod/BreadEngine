#include "viewportWindow.h"
#include "editor.h"
#include "rendering/renderer.h"
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
        constexpr Vector2 permanentSize = {-1, .85f};
        constexpr float verticalPosition = 2;

        UiWindow::awake();
        _content->isActive = false;
        auto horizontalOffset = 0.0f;
        UiButton *gameButton = nullptr;
        UiButton *sceneButton = nullptr;

        sceneButton = &UiPool::buttonPool.get().setup(id + "_sceneButton", getWindowPanel(), "Scene");
        gameButton = &UiPool::buttonPool.get().setup(id + "_gameButton", getWindowPanel(), "Game");
        sceneButton->setSizePercentPermanent(permanentSize);
        sceneButton->setSize({40, -1});
        sceneButton->setPosition({0, verticalPosition});
        sceneButton->setState(STATE_FOCUSED);
        sceneButton->onClick.subscribe([this, gameButton](UiButton *button)
        {
            if (_mode == Scene) return;
            _mode = Scene;
            button->setState(STATE_FOCUSED);
            gameButton->setState(STATE_NORMAL);
        });
        horizontalOffset = sceneButton->getPosition().x + sceneButton->getSize().x;

        gameButton->setSizePercentPermanent(permanentSize);
        gameButton->setSize({40, -1});
        gameButton->setPosition({horizontalOffset + 1, verticalPosition});
        gameButton->onClick.subscribe([this, sceneButton](UiButton *button)
        {
            if (_mode == Game) return;
            _mode = Game;
            button->setState(STATE_FOCUSED);
            sceneButton->setState(STATE_NORMAL);
        });

        const auto playButton = &UiPool::buttonPool.get().setup(id + "_playButton", getWindowPanel(), GuiIconText(ICON_PLAYER_PLAY, nullptr));
        playButton->setAnchor(UI_CENTER_TOP);
        playButton->setSizePercentPermanent(permanentSize);
        playButton->setSize({20, -1});
        playButton->setPosition({-40, verticalPosition});
        playButton->onClick.subscribe([](UiButton *button)
        {
            if (auto &editor = Editor::getInstance(); editor.isPlayMode())
            {
                editor.stopGame();
                button->setState(STATE_NORMAL);
                button->setText(GuiIconText(ICON_PLAYER_PLAY, nullptr));
            }
            else
            {
                editor.runGame();
                button->setState(STATE_FOCUSED);
                button->setText(GuiIconText(ICON_PLAYER_STOP, nullptr));
            }
        });
        horizontalOffset = playButton->getPosition().x + playButton->getSize().x;

        const auto pauseButton = &UiPool::buttonPool.get().setup(id + "_pauseButton", getWindowPanel(), GuiIconText(ICON_PLAYER_PAUSE, nullptr));
        pauseButton->setAnchor(UI_CENTER_TOP);
        pauseButton->setSizePercentPermanent(permanentSize);
        pauseButton->setSize({20, -1});
        pauseButton->setPosition({horizontalOffset + 1, verticalPosition});
        pauseButton->onClick.subscribe([](UiButton *button)
        {
            if (auto &editor = Editor::getInstance(); editor.isPaused())
            {
                editor.resumeGame();
                button->setState(STATE_NORMAL);
            }
            else
            {
                editor.pauseGame();
                button->setState(STATE_FOCUSED);
            }
        });
    }

    void ViewportWindow::draw(const float deltaTime)
    {
        UiWindow::draw(deltaTime);
        BreadEngine::Renderer::get().drawSceneTexture(getViewportSize());

        if (isMouseOver())
        {
            //draw
        }
    }

    void ViewportWindow::update(const float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void ViewportWindow::onFrameEnd(float deltaTime)
    {
        if (const auto isCameraRendered = Editor::getInstance().isCameraRendered(); !isCameraRendered && _warningPanel == nullptr)
        {
            _warningPanel = &UiPool::panelPool.get().setup(id + "_warningPanel", this);
            _warningPanel->setSizePercentPermanent({.2f, .15f});
            _warningPanel->setAnchor(UI_CENTER_CENTER);
            _warningPanel->setPivot({.5f, .5f});
            const auto label = &UiPool::labelPool.get().setup(id + "_warningLabel", _warningPanel, "WARNING!\n\nACTIVE CAMERA NOT FOUND\nSETUP CAMERA IN CAMERA_DIRECTOR COMPONENT");
            label->setSizePercentPermanent({1, 1});
            label->setTextAlignment(TEXT_ALIGN_CENTER);
            label->setTextSize(static_cast<int>(EditorStyle::FontSize::Large));
        }
        else if (isCameraRendered && _warningPanel != nullptr)
        {
            destroyChild(_warningPanel);
            _warningPanel = nullptr;
        }
    }

    void ViewportWindow::dispose()
    {
        _mousePosition = Vector2();
        _warningPanel = nullptr;
        UiWindow::dispose();
    }

    bool ViewportWindow::isMouseOver() const
    {
        return CheckCollisionPointRec(GetMousePosition(), getViewportSize());
    }

    Vector2 ViewportWindow::getMousePosition() const
    {
        if (!isMouseOver()) return (Vector2){-1.0f, -1.0f};

        // The scene target is sized to the panel, so panel-local pixels are scene pixels.
        const auto size = getViewportSize();
        return Vector2Subtract(GetMousePosition(), (Vector2){size.x, size.y});
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
