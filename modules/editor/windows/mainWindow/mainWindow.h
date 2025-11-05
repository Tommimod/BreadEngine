#pragma once
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

        [[nodiscard]] NodeInspector &getNodeInspector();

        void render(float deltaTime);

    private:
        UiToolbar &_toolbar;
        NodeTree _nodeTree;
        NodeInspector *_nodeInspector;
    };
} // namespace BreadEditor
