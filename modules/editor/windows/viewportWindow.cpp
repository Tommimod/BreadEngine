#include "viewportWindow.h"
#include "editor.h"
#include "rendering/renderer.h"
#include "raymath.h"
#include "rlgl.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    namespace {
        /// A point in normalized device space taken back into the world, perspective divide and
        /// all - which raymath's own transform leaves out.
        [[nodiscard]] Vector3 unproject(const Vector3 &normalized, const Matrix &inverseViewProjection)
        {
            const Quaternion transformed = QuaternionTransform(
                Quaternion{normalized.x, normalized.y, normalized.z, 1.0f}, inverseViewProjection);
            if (transformed.w == 0.0f) return Vector3{transformed.x, transformed.y, transformed.z};

            return Vector3{
                transformed.x / transformed.w,
                transformed.y / transformed.w,
                transformed.z / transformed.w
            };
        }
    }

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
        // The scene target is sized to the panel, so panel-local pixels are scene pixels.
        const auto size = getViewportSize();
        return Vector2Subtract(GetMousePosition(), Vector2{size.x, size.y});
    }

    Ray ViewportWindow::getMouseRay() const
    {
        const auto size = getViewportSize();
        const auto mouse = getMousePosition();
        const Matrix inverseViewProjection = MatrixInvert(BreadEngine::Renderer::get().getViewProjection());

        // The projection keeps OpenGL's depth range, so the near plane is at -1 and the far one
        // at +1. Taking the origin from the near plane rather than from the eye is what lets the
        // same two lines serve an orthographic camera, where there is no single eye to take.
        const float normalizedX = 2.0f * mouse.x / size.width - 1.0f;
        const float normalizedY = 1.0f - 2.0f * mouse.y / size.height;
        const Vector3 nearPoint = unproject({normalizedX, normalizedY, -1.0f}, inverseViewProjection);
        const Vector3 farPoint = unproject({normalizedX, normalizedY, 1.0f}, inverseViewProjection);

        return Ray{.position = nearPoint, .direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint))};
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
