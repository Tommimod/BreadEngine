#pragma once
#include "assetsWindow.h"
#include "consoleWindow.h"
#include "gizmoSystem.h"
#include "nodeInspector.h"
#include "nodeTree.h"
#include "../../uitoolkit/uiToolbar.h"
#include "raylib.h"
#include "uitoolkit/UiEmpty.h"
using namespace BreadEngine;

namespace BreadEditor {
    class MainWindow final : UiElement
    {
    public:
        static Vector2 getWindowSize();

        MainWindow();

        ~MainWindow() override;

        [[nodiscard]] UiToolbar &getToolbar() const;

        [[nodiscard]] NodeTree &getNodeTree() const;

        [[nodiscard]] NodeInspector &getNodeInspector() const;

        [[nodiscard]] GizmoSystem &getGizmoSystem();

        [[nodiscard]] AssetsWindow &getAssetsWindow() const;

        [[nodiscard]] ConsoleWindow &getConsoleWindow() const;

        void render3D(float deltaTime);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        unique_ptr<UiEmpty> _leftContainer;
        unique_ptr<UiEmpty> _rightContainer;
        unique_ptr<UiEmpty> _bottomContainer;
        unique_ptr<UiEmpty> _centerContainer;

        GizmoSystem _gizmoSystem;

        [[nodiscard]] UiElement *findUiElementById(const std::string &id) const;
    };
} // namespace BreadEditor
