#pragma once
#include "assetsWindow.h"
#include "consoleWindow.h"
#include "../systems/gizmoSystem.h"
#include "propertyInspectorWindow.h"
#include "nodeTreeWindow.h"
#include "../uitoolkit/uiToolbar.h"
#include "raylib.h"
#include "viewportWindow.h"
#include "systems/mainToolbarSystem.h"
#include "uitoolkit/uiContainer.h"
using namespace BreadEngine;

namespace BreadEditor {
    class MainWindow final : public UiElement
    {
    public:
        static Vector2 getWindowSize();

        MainWindow();

        ~MainWindow() override;

        void initialize();

        void awake() override;

        [[nodiscard]] vector<std::string> &getWindowsOptions();

        [[nodiscard]] UiToolbar &getToolbar() const;

        [[nodiscard]] NodeTreeWindow &getNodeTree() const;

        [[nodiscard]] PropertyInspectorWindow &getPropertyInspector() const;

        [[nodiscard]] GizmoSystem &getGizmoSystem();

        [[nodiscard]] AssetsWindow &getAssetsWindow() const;

        [[nodiscard]] ConsoleWindow &getConsoleWindow() const;

        [[nodiscard]] ViewportWindow &getViewportWindow() const;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void render3D(float deltaTime);

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        unique_ptr<UiContainer> _topLeftContainer;
        unique_ptr<UiContainer> _bottomLeftContainer;
        unique_ptr<UiContainer> _topRightContainer;
        unique_ptr<UiContainer> _bottomRightContainer;
        unique_ptr<UiContainer> _bottomContainer;
        unique_ptr<UiContainer> _centerContainer;

        GizmoSystem _gizmoSystem;
        MainToolbarSystem _mainToolbarSystem;
        vector<std::string> _windowsOptions{};

        [[nodiscard]] UiElement *findUiElementById(const std::string &id) const;

        [[nodiscard]] UiToolbar &getToolbar();

        void resize();
    };
} // namespace BreadEditor
