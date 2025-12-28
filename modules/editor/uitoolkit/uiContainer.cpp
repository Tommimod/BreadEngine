#include "uiContainer.h"
#include "editor.h"
#include "uiPool.h"
#include "windows/mainWindow/mainWindow.h"

namespace BreadEditor {
    UiContainer::UiContainer(const LAYOUT_TYPE layoutType) : _toolbar(UiPool::toolbarPool.get())
    {
        _layoutType = layoutType;
        _toolbar.onButtonPressed.subscribe([this](const int index) { this->onTabChanged(index); });
        addTab();
    }

    UiContainer::~UiContainer()
    = default;

    UiContainer *UiContainer::setup(const std::string &id)
    {
        UiElement::setup(id);
        _toolbar.setup(id + "toolbar", this, _tabs);
        _toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        _toolbar.setPivot({0, 0});
        _toolbar.setSize({0, 20});
        _toolbar.update(0);
        _toolbar.isStatic = true;
        return this;
    }

    UiContainer *UiContainer::setup(const std::string &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        _toolbar.setup(id + "toolbar", this, _tabs);
        _toolbar.setAnchor(UI_FIT_TOP_HORIZONTAL);
        _toolbar.setPivot({0, 0});
        _toolbar.setSize({0, 20});
        _toolbar.update(0);
        _toolbar.isStatic = true;
        return this;
    }

    void UiContainer::draw(const float deltaTime)
    {
        if (!isActive) return;

        UiElement::draw(deltaTime);
    }

    void UiContainer::update(const float deltaTime)
    {
        updateResizable(*this);
        UiElement::update(deltaTime);
    }

    void UiContainer::addChild(UiElement *child)
    {
        UiElement::addChild(child);
        if (getChildCount() == 1) return;
        _tabToWindowIds[_activeTab].emplace_back(child->id);

        recalculateChilds();
    }

    void UiContainer::destroyChild(UiElement *child)
    {
        _tabToWindowIds[_activeTab].erase(ranges::remove(_tabToWindowIds[_activeTab], child->id).begin());
        UiElement::destroyChild(child);
        if (getChildCount() == 1) return;

        recalculateChilds();
    }

    void UiContainer::onTabChanged(const int index)
    {
        if (index == _activeTab) return;

        for (const auto previousTabWindows = &_tabToWindowIds[_activeTab]; const auto &windowId: *previousTabWindows)
        {
            if (const auto child = getChildById(windowId); child != nullptr)
            {
                child->isActive = false;
            }
        }
        _activeTab = index;

        for (const auto currentTabWindows = &_tabToWindowIds[_activeTab]; const auto &windowId: *currentTabWindows)
        {
            if (const auto child = getChildById(windowId); child != nullptr)
            {
                child->isActive = true;
            }
        }
    }

    bool UiContainer::tryDeleteSelf()
    {
        return true;
    }

    void UiContainer::recalculateChilds()
    {
        _toolbarHeightInPercent = _toolbar.getBounds().height / _bounds.height;
        const auto isSingleChild = getChildCount() == 2; //1. toolbar + 2. child
        int i = 0;
        float lastPosition = _toolbar.getSize().y;
        _tabToWindowIds[_activeTab].clear();

        for (const auto &childElement: _childs)
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

            if (const auto window = dynamic_cast<UiWindow *>(childElement); window != nullptr)
            {
                window->setIsCloseable(!isSingleChild);
            }

            _tabToWindowIds[_activeTab].emplace_back(childElement->id);
        }
    }

    void UiContainer::addTab()
    {
        constexpr std::string tabName = "Tab %i";
        _tabs.emplace_back(TextFormat(tabName.c_str(), _tabs.size() + 1));
    }
} // BreadEditor
