#include "uiSide.h"
#include "editor.h"
#include "uiPool.h"
#include "../windows/mainWindow.h"

namespace BreadEditor {
    UiSide::UiSide(const LAYOUT_TYPE layoutType)
    {
        _layoutType = layoutType;
    }

    void UiSide::dispose()
    {
        _slots.clear();
        _topPanel = nullptr;
        _layoutType = LAYOUT_NONE;
        UiScrollPanel::dispose();
    }

    UiSide *UiSide::setup(const std::string_view &id)
    {
        UiScrollPanel::setup(id);
        initializeElements();
        return this;
    }

    UiSide *UiSide::setup(const std::string_view &id, UiElement *parentElement)
    {
        UiScrollPanel::setup(id, parentElement);
        initializeElements();
        return this;
    }

    void UiSide::draw(const float deltaTime)
    {
        _scrollView.width += static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) + static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH)) + 15;
        _scrollView.height += static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) + static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH)) + 1;
        UiScrollPanel::draw(deltaTime);
    }

    void UiSide::update(const float deltaTime)
    {
        updateResizable(*this);
        UiScrollPanel::update(deltaTime);
    }

    void UiSide::addChild(UiElement *child)
    {
        UiScrollPanel::addChild(child);
        if (auto slot = dynamic_cast<UiWindow *>(child); slot != nullptr)
        {
            slot->isActive = true;
            slot->open();
            _slots.emplace_back(slot);
            slot->onClose.subscribe([this](UiWindow *slotForRemove)
            {
                destroyChild(slotForRemove);
            });
            recalculateChilds();
        }
    }

    void UiSide::destroyChild(UiElement *child)
    {
        if (const auto slot = dynamic_cast<UiWindow *>(child); slot != nullptr)
        {
            std::erase(_slots, slot);
            slot->isActive = false;
            slot->onClose.unsubscribeAll();
            recalculateChilds();
            if (_slots.empty()) _contentSize = {0, 0};
        }
        else
        {
            UiScrollPanel::destroyChild(child);
        }
    }

    bool UiSide::tryDeleteSelf()
    {
        return false;
    }

    void UiSide::initializeElements()
    {
        _topPanel = &UiPool::panelPool.get().setup(id + "_topPanel", this);
        _topPanel->setSizePercentPermanent({1, -1});
        _topPanel->setSize({-1, 18});
        _topPanel->setIgnoreScrollLayout();
        _topPanel->setOnOverlayLayer();
        _topPanel->computeBounds();

        const auto addSlotButton = &UiPool::labelButtonPool.get().setup(id + "_addSlotButton", _topPanel, GuiIconText(ICON_BURGER_MENU, nullptr));
        addSlotButton->setTextSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        addSlotButton->setPivot({1, 0});
        addSlotButton->setAnchor(UI_RIGHT_TOP);
        addSlotButton->setSize({10, 10});
        addSlotButton->setPosition({-10, 4});
        addSlotButton->onClick.subscribe([this](UiLabelButton *button)
        {
            auto &model = Editor::getInstance().getEditorModel().getWindowsModel();
            auto &windowsNames = model.getNotOpenedWindowsNames();
            auto dropdown = &UiPool::dropdownPool.get().setup(id + "_dropdown", button, windowsNames, false);
            dropdown->setAnchor(UI_RIGHT_CENTER);
            dropdown->setPivot({1, 0});
            dropdown->setSize({80, 15});
            dropdown->setPosition({-5, 0});
            dropdown->setTextAlignment(TEXT_ALIGN_LEFT);
            dropdown->setOnOverlayLayer();
            dropdown->onOptionSelected.subscribe([this, dropdown, &model, windowsNames, button](const int value)
            {
                button->destroyChild(dropdown);
                if (value >= 1)
                {
                    auto factory = model.getWindowFactory(windowsNames[value - 1]);
                    const auto window = std::invoke(factory);
                    addChild(window);
                }
            });
        });
    }

    void UiSide::recalculateChilds()
    {
        if (getChildCount() == 1) return;
        const float topOffset = _topPanel->getSize().y;
        float lastPosition = 0;

        for (const auto slot: _slots)
        {
            const auto isFirst = _slots.front() == slot;
            const auto isLast = _slots.back() == slot;
            const auto isSingle = _slots.size() == 1;
            if (_layoutType == LAYOUT_HORIZONTAL)
            {
                slot->setLayoutType(LAYOUT_HORIZONTAL);
                slot->setSizePercentPermanent({isSingle ? 1 : -1.0f, 1});
                slot->setSizePercentOneTime({1.0f / static_cast<float>(_slots.size()), -1});
                slot->setSizeMin({150, -1});
                slot->setVerticalResized(false);
                auto side = ANY;
                if (isFirst) side = RIGHT;
                else if (isLast) side = LEFT;
                slot->setHorizontalResized(!isSingle, side);
                slot->setPosition({lastPosition, topOffset});
                lastPosition += slot->getSize().x;
            }
            else if (_layoutType == LAYOUT_VERTICAL)
            {
                slot->setLayoutType(LAYOUT_VERTICAL);
                slot->setSizePercentPermanent({1, isSingle ? 1.0f : -1.0f});
                slot->setSizePercentOneTime({-1, 1.0f / static_cast<float>(_slots.size())});
                slot->setSizeMin({-1, 150});
                slot->setHorizontalResized(false);
                auto side = ANY;
                if (isFirst) side = BOTTOM;
                else if (isLast) side = TOP;
                slot->setVerticalResized(!isSingle, side);
                slot->setPosition({0, lastPosition + topOffset});
                lastPosition += slot->getSize().y;
            }

            calculateRectForScroll(slot);
            setChildLast(slot);
        }
    }
} // BreadEditor
