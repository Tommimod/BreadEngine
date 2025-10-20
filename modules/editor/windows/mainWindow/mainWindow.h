#pragma once

#include "nodeTree.h"
#include "../../uitoolkit/uiToolbar.h"
#include "raylib.h"
using namespace BreadEngine;

namespace BreadEditor
{
    class MainWindow
    {
    public:
        static Vector2 getWindowSize();

        MainWindow();

        ~MainWindow();

        [[nodiscard]] UiToolbar &getToolbar() const;

        [[nodiscard]] NodeTree &getNodeTree();

        void render(float deltaTime);

    private:
        UiToolbar &toolbar;
        NodeTree nodeTree;
    };
} // namespace BreadEditor
