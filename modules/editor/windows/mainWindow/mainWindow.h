#pragma once

#include "nodeInspector.h"
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

        [[nodiscard]] NodeInspector &getNodeInspector();

        void render(float deltaTime);

    private:
        UiToolbar &toolbar;
        NodeInspector nodeInspector;
    };
} // namespace BreadEditor
