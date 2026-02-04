#include "mainWindow.h"
#include "editor.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    Vector2 MainWindow::getWindowSize()
    {
        return Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
    }

    MainWindow::MainWindow() = default;

    MainWindow::~MainWindow()
    {
        dispose();
    }

    void MainWindow::initialize()
    {
        setup("UiRoot", nullptr);
        setAnchor(UI_LEFT_TOP);
        setSizePercentPermanent({1, 1});
        computeBounds();

        const auto &toolbar = getToolbar();

        _leftContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _leftContainer->setPivot({0, 0});
        _leftContainer->setAnchor(UI_FIT_LEFT_VERTICAL);
        _leftContainer->setSizePercentOneTime({.15f, .5f});
        _leftContainer->setPosition({0, toolbar.getSize().y - 1});
        _leftContainer->setHorizontalResized(true);
        _leftContainer->computeBounds();
        _leftContainer->setup("leftContainer", this);

        _leftContainer->addChild(new AssetsWindow(AssetsWindow::Id, _leftContainer.get()));

        _centerContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _centerContainer->setPivot({.5f, 0});
        _centerContainer->setAnchor(UI_CENTER_TOP);
        _centerContainer->setSizePercentOneTime({.7f, .65f});
        _centerContainer->setPosition({0, toolbar.getSize().y - 1});
        _centerContainer->computeBounds();
        _centerContainer->setup("centerContainer", this);

        _centerContainer->addChild(new ViewportWindow(ViewportWindow::Id, _centerContainer.get()));

        _bottomContainer = std::make_unique<UiContainer>(LAYOUT_HORIZONTAL);
        _bottomContainer->setPivot({.5f, 1});
        _bottomContainer->setAnchor(UI_CENTER_BOTTOM);
        _bottomContainer->setSizePercentOneTime({.7f, .35f});
        _bottomContainer->setPosition({0, toolbar.getSize().y - 1});
        _bottomContainer->setVerticalResized(true);
        _bottomContainer->computeBounds();
        _bottomContainer->setup("bottomContainer", this);

        _bottomContainer->addChild(new ConsoleWindow(ConsoleWindow::Id, _bottomContainer.get()));

        _rightContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _rightContainer->setPivot({1, 0});
        _rightContainer->setAnchor(UI_FIT_RIGHT_VERTICAL);
        _rightContainer->setSizePercentOneTime({.15f, .5f});
        _rightContainer->setPosition({0, toolbar.getSize().y - 1});
        _rightContainer->setHorizontalResized(true);
        _rightContainer->computeBounds();
        _rightContainer->setup("rightContainer", this);

        _rightContainer->addChild(new NodeTreeWindow(NodeTreeWindow::Id, _rightContainer.get()));
        _rightContainer->addChild(new NodeInspectorWindow(NodeInspectorWindow::Id, _rightContainer.get()));
    }

    vector<std::string> &MainWindow::getWindowsOptions()
    {
        return _windowsOptions;
    }

    UiToolbar &MainWindow::getToolbar() const
    {
        return dynamic_cast<UiToolbar &>(*findUiElementById("mainWindowToolbar"));
    }

    NodeTreeWindow &MainWindow::getNodeTree() const
    {
        return dynamic_cast<NodeTreeWindow &>(*findUiElementById(NodeTreeWindow::Id));
    }

    NodeInspectorWindow &MainWindow::getNodeInspector() const
    {
        return dynamic_cast<NodeInspectorWindow &>(*findUiElementById(NodeInspectorWindow::Id));
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

    ViewportWindow &MainWindow::getViewportWindow() const
    {
        return dynamic_cast<ViewportWindow &>(*findUiElementById(ViewportWindow::Id));
    }

    void MainWindow::render3D(const float deltaTime)
    {
        if (const auto &model = Editor::getInstance().getEditorModel(); model.getSelectedNodeUiElement() != nullptr)
        {
            _gizmoSystem.render();
        }
    }

    void MainWindow::draw(const float deltaTime)
    {
    }

    void MainWindow::update(const float deltaTime)
    {
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

    UiToolbar &MainWindow::getToolbar()
    {
        auto &categories = _mainToolbarSystem.getCategories();
        auto &toolbar = UiPool::toolbarPool.get().setup("mainWindowToolbar", this, categories, true);
        toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        toolbar.setPivot({0, 0});
        toolbar.setSize({0, 20});
        toolbar.computeBounds();
        toolbar.setOnOverlayLayer();
        toolbar.isStatic = true;

        toolbar.onButtonPressed.subscribe([this, &toolbar, &categories](int index)
        {
            auto &nextOptions = _mainToolbarSystem.getOptions(categories[index]);
            std::vector<std::string> options{};
            options.reserve(nextOptions.size());
            for (const auto &nextOption: nextOptions)
            {
                options.push_back(nextOption.optionName);
            }

            auto &dropdown = UiPool::dropdownPool.get().setup(toolbar.id + "toolbarDropdown", &toolbar, options, false);
            dropdown.setAnchor(UI_LEFT_CENTER);
            dropdown.setPivot({0, 0});
            dropdown.setSize({80, 15});
            dropdown.setPosition({toolbar.getAllChilds()[index]->getPosition().x, 0});
            dropdown.setTextAlignment(TEXT_ALIGN_LEFT);
            dropdown.onValueChanged.subscribe([&dropdown, &toolbar, this, &index, &categories](const int value)
            {
                toolbar.destroyChild(&dropdown);
                if (value >= 1)
                {
                    _mainToolbarSystem.processCommand(categories[index], value);
                }
            });
        });
        return toolbar;
    }
} // namespace BreadEditor
