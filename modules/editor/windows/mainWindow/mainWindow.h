#pragma once
#include "assetsWindow.h"
#include "consoleWindow.h"
#include "gizmoSystem.h"
#include "nodeInspector.h"
#include "nodeTree.h"
#include "../../uitoolkit/uiToolbar.h"
#include "raylib.h"
using namespace BreadEngine;

namespace BreadEditor {
    class MainWindow
    {
    public:
        static Vector2 getWindowSize();

        MainWindow();

        ~MainWindow();

        [[nodiscard]] UiToolbar &getToolbar() const;

        [[nodiscard]] NodeTree &getNodeTree();

        [[nodiscard]] NodeInspector &getNodeInspector() const;

        [[nodiscard]] GizmoSystem &getGizmoSystem();

        [[nodiscard]] AssetsWindow &getAssetsWindow();

        [[nodiscard]] ConsoleWindow &getConsoleWindow();

        void render2D(float deltaTime);

        void render3D(float deltaTime);

    private:
        GizmoSystem _gizmoSystem;
        UiToolbar &_toolbar;
        NodeTree _nodeTree;
        NodeInspector *_nodeInspector;
        AssetsWindow _assetsWindow;
        ConsoleWindow _consoleWindow;
    };
} // namespace BreadEditor
