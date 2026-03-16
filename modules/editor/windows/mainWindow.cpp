#include "mainWindow.h"
#include <thread>
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

        _topLeftContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _topLeftContainer->setPivot({0, 0});
        _topLeftContainer->setAnchor(UI_FIT_LEFT_VERTICAL);
        _topLeftContainer->setSizePercentOneTime({.15f, 1});
        _topLeftContainer->setPosition({0, toolbar.getSize().y - 1});
        _topLeftContainer->setHorizontalResized(true);
        _topLeftContainer->computeBounds();
        _topLeftContainer->setup("topLeftContainer", this);

        _topLeftContainer->addChild(new AssetsWindow(AssetsWindow::Id));

        _bottomLeftContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _bottomLeftContainer->setPivot({1, 1});
        _bottomLeftContainer->setAnchor(UI_FIT_BOTTOM_HORIZONTAL);
        _bottomLeftContainer->setSizePercentPermanent({1, -1});
        _bottomLeftContainer->setSizePercentOneTime({-1, .5f});
        _bottomLeftContainer->setPosition({0, toolbar.getSize().y - 1});
        _bottomLeftContainer->setVerticalResized(true);
        _bottomLeftContainer->computeBounds();
        _bottomLeftContainer->setup("bottomLeftContainer", _topLeftContainer.get());

        _centerContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _centerContainer->setPivot({.5f, 0});
        _centerContainer->setAnchor(UI_CENTER_TOP);
        _centerContainer->setSizePercentOneTime({.7f, .65f});
        _centerContainer->setPosition({0, toolbar.getSize().y - 1});
        _centerContainer->computeBounds();
        _centerContainer->setup("centerContainer", this);

        _centerContainer->addChild(new ViewportWindow(ViewportWindow::Id));

        _bottomContainer = std::make_unique<UiContainer>(LAYOUT_HORIZONTAL);
        _bottomContainer->setPivot({.5f, 1});
        _bottomContainer->setAnchor(UI_CENTER_BOTTOM);
        _bottomContainer->setSizePercentOneTime({.7f, .35f});
        _bottomContainer->setPosition({0, toolbar.getSize().y - 1});
        _bottomContainer->setVerticalResized(true);
        _bottomContainer->computeBounds();
        _bottomContainer->setup("bottomContainer", this);

        _bottomContainer->addChild(new ConsoleWindow(ConsoleWindow::Id));

        _topRightContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _topRightContainer->setPivot({1, 0});
        _topRightContainer->setAnchor(UI_FIT_RIGHT_VERTICAL);
        _topRightContainer->setSizePercentOneTime({.15f, 1});
        _topRightContainer->setPosition({0, toolbar.getSize().y - 1});
        _topRightContainer->setHorizontalResized(true);
        _topRightContainer->computeBounds();
        _topRightContainer->setup("topRightContainer", this);

        _topRightContainer->addChild(new NodeTreeWindow(NodeTreeWindow::Id));

        _bottomRightContainer = std::make_unique<UiContainer>(LAYOUT_VERTICAL);
        _bottomRightContainer->setPivot({1, 1});
        _bottomRightContainer->setAnchor(UI_FIT_BOTTOM_HORIZONTAL);
        _bottomRightContainer->setSizePercentPermanent({1, -1});
        _bottomRightContainer->setSizePercentOneTime({-1, .5f});
        _bottomRightContainer->setPosition({0, toolbar.getSize().y - 1});
        _bottomRightContainer->setVerticalResized(true);
        _bottomRightContainer->computeBounds();
        _bottomRightContainer->setup("bottomRightContainer", _topRightContainer.get());

        _bottomRightContainer->addChild(new PropertyInspectorWindow(PropertyInspectorWindow::Id));
    }

    void MainWindow::awake()
    {
        std::thread workerThread([this]
        {
            constexpr auto time = std::chrono::milliseconds(1);
            while (!Engine::shouldClose())
            {
                std::this_thread::sleep_for(time);
                if (IsWindowResized())
                {
                    resize();
                }
            }
        });

        workerThread.detach();
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

    PropertyInspectorWindow &MainWindow::getPropertyInspector() const
    {
        return dynamic_cast<PropertyInspectorWindow &>(*findUiElementById(PropertyInspectorWindow::Id));
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
        UiElement *element = _topLeftContainer->getChildById(id);
        if (element != nullptr) return element;

        element = _bottomLeftContainer->getChildById(id);
        if (element != nullptr) return element;

        element = _topRightContainer->getChildById(id);
        if (element != nullptr) return element;

        element = _bottomRightContainer->getChildById(id);
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

        toolbar.onButtonPressed.subscribe([this, &toolbar, &categories](const UiElement *uiElement)
        {
            auto index = uiElement->getIndex();
            const auto &nextOptions = _mainToolbarSystem.getOptions(categories[index]);
            std::vector<std::string> options{};
            options.reserve(nextOptions.size());
            for (const auto &nextOption: nextOptions)
            {
                options.push_back(nextOption.optionName);
            }

            auto &dropdown = UiPool::dropdownPool.get().setup(id + "_toolbarDropdown", &toolbar, options, false);
            dropdown.setAnchor(UI_LEFT_CENTER);
            dropdown.setPivot({0, 0});
            dropdown.setSize({80, 15});
            dropdown.setPosition({toolbar.getAllChilds()[index]->getPosition().x, 0});
            dropdown.setTextAlignment(TEXT_ALIGN_LEFT);
            dropdown.onOptionSelected.subscribe([&dropdown, &toolbar, this, &index, &categories](const int value)
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

    void MainWindow::resize()
    {
        setSizePercentPermanent({1, 1});
        computeBounds();

        _topLeftContainer->setSizePercentOneTime({.15f, 1});
        _bottomLeftContainer->setSizePercentOneTime({1, .5f});
        _centerContainer->setSizePercentOneTime({.7f, .65f});
        _bottomContainer->setSizePercentOneTime({.7f, .35f});
        _topRightContainer->setSizePercentOneTime({.15f, 1});
        _bottomRightContainer->setSizePercentOneTime({1, .5f});
    }
} // namespace BreadEditor
