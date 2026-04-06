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

    MainWindow::~MainWindow() = default;

    void MainWindow::initialize()
    {
        setup("UiRoot", nullptr);
        setAnchor(UI_LEFT_TOP);
        setSizePercentPermanent({1, 1});
        computeBounds();

        const auto &toolbar = getToolbar();
        auto &windowsModel = Editor::getInstance().getEditorModel().getWindowsModel();
        windowsModel.initialize();

        _leftSide = std::make_unique<UiSide>(LAYOUT_VERTICAL);
        _leftSide->setPivot({0, 0});
        _leftSide->setAnchor(UI_FIT_LEFT_VERTICAL);
        _leftSide->setSizePercentOneTime({.15f, 1});
        _leftSide->setPosition({0, toolbar.getSize().y - 1});
        _leftSide->setHorizontalResized(true, IUiResizable::RIGHT);
        _leftSide->computeBounds();
        _leftSide->setup("leftSide", this);
        _leftSide->addChild(windowsModel.getWindowFactory(AssetsWindow::Id)());

        _centerSide = std::make_unique<UiSide>(LAYOUT_VERTICAL);
        _centerSide->setPivot({.5f, 0});
        _centerSide->setAnchor(UI_CENTER_TOP);
        _centerSide->setSizePercentOneTime({.7f, .65f});
        _centerSide->setPosition({0, toolbar.getSize().y - 1});
        _centerSide->computeBounds();
        _centerSide->setup("topSide", this);
        _centerSide->addChild(windowsModel.getWindowFactory(ViewportWindow::Id)());

        _bottomSide = std::make_unique<UiSide>(LAYOUT_HORIZONTAL);
        _bottomSide->setPivot({.5f, 1});
        _bottomSide->setAnchor(UI_CENTER_BOTTOM);
        _bottomSide->setSizePercentOneTime({.7f, .35f});
        _bottomSide->setPosition({0, toolbar.getSize().y - 1});
        _bottomSide->setVerticalResized(true, IUiResizable::TOP);
        _bottomSide->computeBounds();
        _bottomSide->setup("bottomSide", this);
        _bottomSide->addChild(windowsModel.getWindowFactory(ConsoleWindow::Id)());

        _rightSide = std::make_unique<UiSide>(LAYOUT_VERTICAL);
        _rightSide->setPivot({1, 0});
        _rightSide->setAnchor(UI_FIT_RIGHT_VERTICAL);
        _rightSide->setSizePercentOneTime({.15f, 1});
        _rightSide->setPosition({0, toolbar.getSize().y - 1});
        _rightSide->setHorizontalResized(true, IUiResizable::LEFT);
        _rightSide->computeBounds();
        _rightSide->setup("rightSide", this);
        _rightSide->addChild(windowsModel.getWindowFactory(NodeTreeWindow::Id)());
        _rightSide->addChild(windowsModel.getWindowFactory(PropertyInspectorWindow::Id)());
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
        if (const auto &model = Editor::getInstance().getEditorModel(); model.getSelectedEngineNode() != nullptr)
        {
            _gizmoSystem.render();
        }
    }

    bool MainWindow::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }

    UiElement *MainWindow::findUiElementById(const std::string &id) const
    {
        std::vector<UiElement *> slots;
        slots.emplace_back(_leftSide.get());
        slots.emplace_back(_rightSide.get());
        slots.emplace_back(_bottomSide.get());
        slots.emplace_back(_centerSide.get());
        for (const auto slot: slots)
        {
            if (const auto element = slot->getChildById(id); element != nullptr)
            {
                return element;
            }
        }

        return nullptr;
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
            dropdown.setOnOverlayLayer();
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

        _leftSide->setSizePercentOneTime(_leftSide->getSizeInPercent());
        _centerSide->setSizePercentOneTime(_centerSide->getSizeInPercent());
        _bottomSide->setSizePercentOneTime(_bottomSide->getSizeInPercent());
        _rightSide->setSizePercentOneTime(_rightSide->getSizeInPercent());
    }
} // namespace BreadEditor
