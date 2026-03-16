#include "uiContainer.h"
#include "editor.h"
#include "uiPool.h"
#include "../windows/mainWindow.h"

namespace BreadEditor {
    UiContainer::UiContainer(const LAYOUT_TYPE layoutType)
    {
        _layoutType = layoutType;
    }

    void UiContainer::dispose()
    {
        _tabs.clear();
        _toolbar = nullptr;
        UiElement::dispose();
    }

    UiContainer *UiContainer::setup(const std::string_view &id)
    {
        UiElement::setup(id);
        initialize();
        return this;
    }

    UiContainer *UiContainer::setup(const std::string_view &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        initialize();
        return this;
    }

    void UiContainer::draw(const float deltaTime)
    {
        if (_tabs.empty())
        {
            GuiPanel(_bounds, nullptr);
        }
    }

    void UiContainer::update(const float deltaTime)
    {
        updateResizable(*this);
    }

    void UiContainer::addChild(UiElement *child)
    {
        UiElement::addChild(child);
        const auto window = dynamic_cast<UiWindow *>(child);
        if (window == nullptr) return;
        if (getChildCount() == 1) return;

        _tabs.emplace_back(child->id);
        _toolbar->replaceButtons(_tabs);
        const auto index = getChildCount() - 1;
        const auto &id = _toolbar->getAllChilds()[index]->id;
        _tabIdToWindow[id] = window;
        recalculateChilds();
    }

    void UiContainer::destroyChild(UiElement *child)
    {
        if (dynamic_cast<UiWindow *>(child) == nullptr)
        {
            UiElement::destroyChild(child);
            return;
        }

        _tabs.erase(std::ranges::remove(_tabs, child->id).begin());
        _toolbar->replaceButtons(_tabs);
        UiElement::destroyChild(child);
        if (_tabs.empty()) return;

        recalculateChilds();
    }

    void UiContainer::onTabChanged(UiElement *uiElement)
    {
    }

    void UiContainer::onTabClosed(const UiElement *uiElement)
    {
        const auto window = _tabIdToWindow[uiElement->id];
        _tabIdToWindow.erase(uiElement->id);
        window->close();
    }

    bool UiContainer::tryDeleteSelf()
    {
        return true;
    }

    void UiContainer::initialize()
    {
        _toolbar = &UiPool::toolbarPool.get().setup(id + "_toolbar", this, _tabs);
        _toolbar->setAnchor(UI_FIT_TOP_HORIZONTAL);
        _toolbar->setPivot({0, 0});
        _toolbar->setSize({0, 20});
        _toolbar->computeBounds();
        _toolbar->onButtonPressed.subscribe([this](UiElement *uiElement) { onTabChanged(uiElement); });
        _toolbar->onButtonRequestedToRemove.subscribe([this](const UiElement *uiElement) { onTabClosed(uiElement); });
        _toolbar->isStatic = true;

        const auto toolbarOptButton = &UiPool::labelButtonPool.get().setup(id + "_optButton", _toolbar, GuiIconText(ICON_BURGER_MENU, nullptr));
        toolbarOptButton->setTextAlignment(TEXT_ALIGN_CENTER);
        toolbarOptButton->setAnchor(UI_RIGHT_CENTER);
        toolbarOptButton->setPivot({1, .5f});
        toolbarOptButton->setSize({20, 20});
        toolbarOptButton->setPosition({-5, 0});
        toolbarOptButton->onClick.subscribe([this](UiLabelButton *)
        {
            auto model = Editor::getInstance().getEditorModel().getWindowsModel();
            auto windowsNames = model->getNotOpenedWindowsNames();
            auto dropdown = &UiPool::dropdownPool.get().setup(id + "_dropdown", _toolbar, windowsNames, false);
            dropdown->setAnchor(UI_RIGHT_CENTER);
            dropdown->setPivot({1, 0});
            dropdown->setSize({80, 15});
            dropdown->setPosition({-5, 0});
            dropdown->setTextAlignment(TEXT_ALIGN_LEFT);
            dropdown->setOnOverlayLayer();
            dropdown->onOptionSelected.subscribe([this, dropdown, model, windowsNames](const int value)
            {
                _toolbar->destroyChild(dropdown);
                if (value >= 1)
                {
                    auto factory = model->getWindowFactory(windowsNames[value - 1]);
                    const auto window = std::invoke(factory);
                    addChild(window);
                }
            });
        });
    }

    void UiContainer::recalculateChilds()
    {
        if (getChildCount() == 1) return;
        const auto isSingleChild = getChildCount() == 2; //1. toolbar + 2. child
        int i = 0;
        float lastPosition = _toolbar->getSize().y;

        for (const auto &childElement: getAllChilds())
        {
            if (childElement->isStatic) continue;
            if (_layoutType == LAYOUT_HORIZONTAL)
            {
                if (isSingleChild)
                {
                    childElement->setSizePercentPermanent({1, 1});
                }
                else
                {
                    const auto childCount = getChildCount() - 1;
                    childElement->setSizePercentPermanent({-1, 1});
                    childElement->setSizePercentOneTime({1.0f / static_cast<float>(childCount), 1.0f});
                }

                if (const auto resizableElement = dynamic_cast<IUiResizable *>(childElement); resizableElement != nullptr)
                {
                    resizableElement->setHorizontalResized(i != 0);
                }
            }
            else if (_layoutType == LAYOUT_VERTICAL)
            {
                if (isSingleChild)
                {
                    childElement->setSizePercentPermanent({1, 1});
                }
                else
                {
                    const auto childCount = getChildCount() - 1;
                    childElement->setSizePercentPermanent({1, -1});
                    childElement->setSizePercentOneTime({1.0f, 1.0f / static_cast<float>(childCount)});
                }

                if (const auto resizableElement = dynamic_cast<IUiResizable *>(childElement); resizableElement != nullptr)
                {
                    resizableElement->setVerticalResized(i != 0);
                }
            }

            childElement->setPosition({0, lastPosition});
            lastPosition += childElement->getSize().y;
            i++;
        }

        setChildLast(_toolbar);
    }
} // BreadEditor
