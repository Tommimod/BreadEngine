#pragma once
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

        void render(float deltaTime);

    private:
        GizmoSystem _gizmoSystem;
        UiToolbar &_toolbar;
        NodeTree _nodeTree;
        NodeInspector *_nodeInspector;
    };
} // namespace BreadEditor
