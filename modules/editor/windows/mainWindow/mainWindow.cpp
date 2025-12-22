#include "mainWindow.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    Vector2 MainWindow::getWindowSize()
    {
        return Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
    }

    MainWindow::MainWindow()
        : _toolbar(UiPool::toolbarPool.get().setup("mainWindowToolbar", nullptr, 20, {"File", "Edit", "Help"})),
          _nodeTree(NodeTree(NodeTree::Id)),
          _assetsWindow(AssetsWindow(AssetsWindow::Id)),
          _consoleWindow(ConsoleWindow(ConsoleWindow::Id))
    {
        _toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        _toolbar.setPivot({0, 0});
        _toolbar.setSize({0, 20});

        _nodeTree.setPivot({1, 0});
        _nodeTree.setAnchor(UI_FIT_RIGHT_VERTICAL);
        _nodeTree.setSizePercentOneTime({.15f, .5f});
        _nodeTree.setPosition({0, _toolbar.getSize().y - 1});

        _nodeInspector = new NodeInspector(NodeInspector::Id, &_nodeTree);
        _nodeInspector->setPivot({1, 1});
        _nodeInspector->setAnchor(UI_RIGHT_BOTTOM);
        _nodeInspector->setSizePercentOneTime({1, .5f});
        _nodeInspector->setSizePercentPermanent({1, -1});
        _nodeInspector->setPosition({0, _toolbar.getSize().y - 1});

        _assetsWindow.setPivot({0, 0});
        _assetsWindow.setAnchor(UI_FIT_LEFT_VERTICAL);
        _assetsWindow.setSizePercentOneTime({.15f, .5f});
        _assetsWindow.setPosition({0, _toolbar.getSize().y - 1});
        _assetsWindow.update(0);

        _consoleWindow.setPivot({0, 1.0f});
        _consoleWindow.setAnchor(UI_LEFT_BOTTOM);
        auto size = getWindowSize();
        size.x -= _assetsWindow.getSize().x;
        size.x -= _nodeTree.getSize().x;
        size.y *= .3f;
        _consoleWindow.setSize(size);
        _consoleWindow.setPosition({_assetsWindow.getBounds().width, 100});
    }

    MainWindow::~MainWindow()
    {
        UiPool::toolbarPool.release(_toolbar);
    }

    UiToolbar &MainWindow::getToolbar() const
    {
        return _toolbar;
    }

    NodeTree &MainWindow::getNodeTree()
    {
        return _nodeTree;
    }

    NodeInspector &MainWindow::getNodeInspector() const
    {
        return *_nodeInspector;
    }

    GizmoSystem &MainWindow::getGizmoSystem()
    {
        return _gizmoSystem;
    }

    AssetsWindow &MainWindow::getAssetsWindow()
    {
        return _assetsWindow;
    }

    ConsoleWindow &MainWindow::getConsoleWindow()
    {
        return _consoleWindow;
    }

    void MainWindow::render2D(const float deltaTime)
    {
        _toolbar.update(deltaTime);
        _toolbar.draw(deltaTime);

        _nodeTree.update(deltaTime);
        _nodeTree.draw(deltaTime);

        _assetsWindow.update(deltaTime);
        _assetsWindow.draw(deltaTime);

        _consoleWindow.update(deltaTime);
        _consoleWindow.draw(deltaTime);
    }

    void MainWindow::render3D(float deltaTime)
    {
        if (const auto selectedNodeUiElement = _nodeTree.getSelectedNodeUiElement(); selectedNodeUiElement != nullptr)
        {
            _gizmoSystem.render();
        }
    }
} // namespace BreadEditor
