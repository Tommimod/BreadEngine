#include "mainWindow.h"

#include <iostream>
#include <ostream>

#include "uitoolkit/uiPool.h"

namespace BreadEditor
{
    Vector2 MainWindow::getWindowSize()
    {
        return Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
    }

    MainWindow::MainWindow()
        : toolbar(UiPool::toolbarPool.get().setup("mainWindowToolbar", nullptr, 20, {"File", "Edit", "Help"})),
          nodeInspector(NodeInspector("mainWindowNodeInspector"))
    {
        toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        toolbar.setPivot({0, 0});
        toolbar.setSize({0, 20});

        nodeInspector.setPivot({1, 0});
        nodeInspector.setAnchor(UI_FIT_RIGHT_VERTICAL);
        nodeInspector.setSizePercent({.3f, 0});
        nodeInspector.setPosition({0, toolbar.getSize().y - 1});
    }

    MainWindow::~MainWindow()
    {
        UiPool::toolbarPool.release(toolbar);
    }

    UiToolbar &MainWindow::getToolbar() const
    {
        return toolbar;
    }

    NodeInspector &MainWindow::getNodeInspector()
    {
        return nodeInspector;
    }

    void MainWindow::render(const float deltaTime)
    {
        toolbar.update(deltaTime);
        toolbar.draw(deltaTime);

        nodeInspector.update(deltaTime);
        nodeInspector.draw(deltaTime);
    }
} // namespace BreadEditor
