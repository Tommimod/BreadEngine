#include "mainWindow.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    Vector2 MainWindow::getWindowSize()
    {
        return Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
    }

    MainWindow::MainWindow()
        : toolbar(UiPool::toolbarPool.get().setup("mainWindowToolbar", nullptr, 20, {"File", "Edit", "Help"})),
          nodeTree(NodeTree("mainWindowNodeTree"))
    {
        toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        toolbar.setPivot({0, 0});
        toolbar.setSize({0, 20});

        nodeTree.setPivot({1, 0});
        nodeTree.setAnchor(UI_FIT_RIGHT_VERTICAL);
        nodeTree.setSizePercentOneTime({.3f, .5f});
        nodeTree.setPosition({0, toolbar.getSize().y - 1});

        nodeInspector = new NodeInspector("mainWindowNodeInspector", &nodeTree);
        nodeInspector->setPivot({1, 1});
        nodeInspector->setAnchor(UI_RIGHT_BOTTOM);
        nodeInspector->setSizePercentOneTime({1, .5f});
        nodeInspector->setSizePercentPermanent({1, -1});
        nodeInspector->setPosition({0, toolbar.getSize().y - 1});
    }

    MainWindow::~MainWindow()
    {
        UiPool::toolbarPool.release(toolbar);
    }

    UiToolbar &MainWindow::getToolbar() const
    {
        return toolbar;
    }

    NodeTree &MainWindow::getNodeTree()
    {
        return nodeTree;
    }

    NodeInspector & MainWindow::getNodeInspector()
    {
        return *nodeInspector;
    }

    void MainWindow::render(const float deltaTime)
    {
        toolbar.update(deltaTime);
        toolbar.draw(deltaTime);

        nodeTree.update(deltaTime);
        nodeTree.draw(deltaTime);
    }
} // namespace BreadEditor
