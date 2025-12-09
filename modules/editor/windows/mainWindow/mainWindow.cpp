#include "mainWindow.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    Vector2 MainWindow::getWindowSize()
    {
        return Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
    }

    MainWindow::MainWindow()
        : _gizmoSystem(),
          _toolbar(UiPool::toolbarPool.get().setup("mainWindowToolbar", nullptr, 20, {"File", "Edit", "Help"})),
          _nodeTree(NodeTree(NodeTree::Id))
    {
        _toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        _toolbar.setPivot({0, 0});
        _toolbar.setSize({0, 20});

        _nodeTree.setPivot({1, 0});
        _nodeTree.setAnchor(UI_FIT_RIGHT_VERTICAL);
        _nodeTree.setSizePercentOneTime({.3f, .5f});
        _nodeTree.setPosition({0, _toolbar.getSize().y - 1});

        _nodeInspector = new NodeInspector(NodeInspector::Id, &_nodeTree);
        _nodeInspector->setPivot({1, 1});
        _nodeInspector->setAnchor(UI_RIGHT_BOTTOM);
        _nodeInspector->setSizePercentOneTime({1, .5f});
        _nodeInspector->setSizePercentPermanent({1, -1});
        _nodeInspector->setPosition({0, _toolbar.getSize().y - 1});
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

    void MainWindow::render(const float deltaTime)
    {
        _toolbar.update(deltaTime);
        _toolbar.draw(deltaTime);

        _nodeTree.update(deltaTime);
        _nodeTree.draw(deltaTime);

        if (const auto selectedNodeUiElement = _nodeTree.getSelectedNodeUiElement(); selectedNodeUiElement != nullptr)
        {
            _gizmoSystem.Draw(selectedNodeUiElement->getNode()->get<BreadEngine::Transform>());
        }
    }
} // namespace BreadEditor
