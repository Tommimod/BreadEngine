#pragma once
#include "assetsWindow.h"
#include "consoleWindow.h"
#include "gizmoSystem.h"
#include "nodeInspectorWindow.h"
#include "nodeTreeWindow.h"
#include "../../uitoolkit/uiToolbar.h"
#include "raylib.h"
#include "viewportWindow.h"
#include "uitoolkit/uiContainer.h"
using namespace BreadEngine;

namespace BreadEditor {
    class MainWindow final : UiElement
    {
    public:
        static Vector2 getWindowSize();

        MainWindow();

        ~MainWindow() override;

        void initialize();

        [[nodiscard]] vector<std::string> &getWindowsOptions();

        [[nodiscard]] UiToolbar &getToolbar() const;

        [[nodiscard]] NodeTreeWindow &getNodeTree() const;

        [[nodiscard]] NodeInspectorWindow &getNodeInspector() const;

        [[nodiscard]] GizmoSystem &getGizmoSystem();

        [[nodiscard]] AssetsWindow &getAssetsWindow() const;

        [[nodiscard]] ConsoleWindow &getConsoleWindow() const;

        [[nodiscard]] ViewportWindow &getViewportWindow() const;

        void render3D(float deltaTime);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        unique_ptr<UiContainer> _leftContainer;
        unique_ptr<UiContainer> _rightContainer;
        unique_ptr<UiContainer> _bottomContainer;
        unique_ptr<UiContainer> _centerContainer;

        GizmoSystem _gizmoSystem;
        vector<std::string> _windowsOptions {};

        [[nodiscard]] UiElement *findUiElementById(const std::string &id) const;
    };
} // namespace BreadEditor
