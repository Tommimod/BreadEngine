#include "mainWindow.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    Vector2 MainWindow::getWindowSize()
    {
        return Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
    }

    MainWindow::MainWindow()
    {
        setup("UiRoot", nullptr);
        setAnchor(UI_LEFT_TOP);
        setSizePercentPermanent({1, 1});
        update(0);

        const auto toolbar = &UiPool::toolbarPool.get().setup("mainWindowToolbar", this, 20, {"File", "Edit", "Help"});
        addChild(toolbar);
        toolbar->setAnchor(UI_FIT_TOP_HORIZONTAL);
        toolbar->setPivot({0, 0});
        toolbar->setSize({0, 20});
        toolbar->update(0);
        toolbar->isStatic = true;

        _leftContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _leftContainer->setup("leftContainer", this);
        _leftContainer->setPivot({0, 0});
        _leftContainer->setAnchor(UI_FIT_LEFT_VERTICAL);
        _leftContainer->setSizePercentOneTime({.15f, .5f});
        _leftContainer->setPosition({0, toolbar->getSize().y - 1});
        _leftContainer->setHorizontalResized(true);
        _leftContainer->update(0);

        const auto assetWindow = new AssetsWindow(AssetsWindow::Id);
        _leftContainer->addChild(assetWindow);
        assetWindow->setSizePercentOneTime({1, 1});
        assetWindow->setSizePercentPermanent({1, -1});
        assetWindow->setPosition({0, 0});
        assetWindow->update(0);

        _bottomContainer = std::make_unique<UiContainer>(LAYOUT_HORIZONTAL);
        _bottomContainer->setup("bottomContainer", this);
        _bottomContainer->setPivot({.5f, 1});
        _bottomContainer->setAnchor(UI_CENTER_BOTTOM);
        _bottomContainer->setSizePercentOneTime({.7f, .35f});
        _bottomContainer->setPosition({0, toolbar->getSize().y - 1});
        _bottomContainer->setVerticalResized(true);
        _bottomContainer->update(0);

        const auto consoleWindow = new ConsoleWindow(ConsoleWindow::Id);
        _bottomContainer->addChild(consoleWindow);
        consoleWindow->setSizePercentOneTime({1, 1});
        consoleWindow->setSizePercentPermanent({1, 1});
        consoleWindow->setPosition({0, 0});
        consoleWindow->update(0);

        _centerContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _centerContainer->setup("centerContainer", this);
        _centerContainer->setPivot({.5f, 0});
        _centerContainer->setAnchor(UI_CENTER_TOP);
        _centerContainer->setSizePercentOneTime({.7f, .65f});
        _centerContainer->setPosition({0, toolbar->getSize().y - 1});
        _centerContainer->update(0);

        const auto viewportWindow = new ViewportWindow(ViewportWindow::Id);
        _centerContainer->addChild(viewportWindow);
        viewportWindow->setSizePercentOneTime({1, 1});
        viewportWindow->setSizePercentPermanent({1, 1});
        viewportWindow->setPosition({0, 0});
        viewportWindow->update(0);

        _rightContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _rightContainer->setup("rightContainer", this);
        _rightContainer->setPivot({1, 0});
        _rightContainer->setAnchor(UI_FIT_RIGHT_VERTICAL);
        _rightContainer->setSizePercentOneTime({.15f, .5f});
        _rightContainer->setPosition({0, toolbar->getSize().y - 1});
        _rightContainer->setHorizontalResized(true);
        _rightContainer->update(0);

        const auto nodeTree = new NodeTree(NodeTree::Id);
        _rightContainer->addChild(nodeTree);
        nodeTree->setSizePercentOneTime({1, .5f});
        nodeTree->setSizePercentPermanent({1, -1});
        nodeTree->setPosition({0, 0});

        const auto nodeInspector = new NodeInspector(NodeInspector::Id);
        _rightContainer->addChild(nodeInspector);
        nodeInspector->setSizePercentOneTime({1, .5f});
        nodeInspector->setSizePercentPermanent({1, -1});
        nodeInspector->setPosition({0, nodeTree->getSize().y});
        nodeInspector->setVerticalResized(true);
    }

    MainWindow::~MainWindow()
    {
        dispose();
    }

    UiToolbar &MainWindow::getToolbar() const
    {
        return dynamic_cast<UiToolbar &>(*findUiElementById("mainWindowToolbar"));
    }

    NodeTree &MainWindow::getNodeTree() const
    {
        return dynamic_cast<NodeTree &>(*findUiElementById(NodeTree::Id));
    }

    NodeInspector &MainWindow::getNodeInspector() const
    {
        return dynamic_cast<NodeInspector &>(*findUiElementById(NodeInspector::Id));
    }

    GizmoSystem &MainWindow::getGizmoSystem()
    {
        return _gizmoSystem;
    }

    AssetsWindow &MainWindow::getAssetsWindow() const
    {
        return dynamic_cast<AssetsWindow &>(*findUiElementById(AssetsWindow::Id));
    }

    ConsoleWindow &MainWindow::getConsoleWindow() const
    {
        return dynamic_cast<ConsoleWindow &>(*findUiElementById(ConsoleWindow::Id));
    }

    ViewportWindow & MainWindow::getViewportWindow() const
    {
        return dynamic_cast<ViewportWindow &>(*findUiElementById(ViewportWindow::Id));
    }

    void MainWindow::render3D(float deltaTime)
    {
        if (const auto selectedNodeUiElement = getNodeTree().getSelectedNodeUiElement(); selectedNodeUiElement != nullptr)
        {
            _gizmoSystem.render();
        }
    }

    void MainWindow::draw(float deltaTime)
    {
        UiElement::draw(deltaTime);
    }

    void MainWindow::update(float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void MainWindow::dispose()
    {
        UiElement::dispose();
    }

    bool MainWindow::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }

    UiElement *MainWindow::findUiElementById(const std::string &id) const
    {
        UiElement *element = _leftContainer->getChildById(id);
        if (element != nullptr) return element;

        element = _rightContainer->getChildById(id);
        if (element != nullptr) return element;

        element = _bottomContainer->getChildById(id);
        if (element != nullptr) return element;

        element = _centerContainer->getChildById(id);
        if (element != nullptr) return element;

        return element;
    }
} // namespace BreadEditor
