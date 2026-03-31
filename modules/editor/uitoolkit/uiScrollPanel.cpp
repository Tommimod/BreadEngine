#include "uiScrollPanel.h"
#include "uiPool.h"

namespace BreadEditor {
    UiScrollPanel::UiScrollPanel() = default;

    UiScrollPanel::~UiScrollPanel() = default;

    UiScrollPanel &UiScrollPanel::setup(const std::string_view &id)
    {
        UiElement::setup(id);
        return *this;
    }

    UiScrollPanel &UiScrollPanel::setup(const std::string_view &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiScrollPanel::dispose()
    {
        _contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        _scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        _scrollPos = {0.0f, 0.0f};
        _contentSize = {0, 0};
        _title = nullptr;
        UiElement::dispose();
    }

    void UiScrollPanel::draw(float deltaTime)
    {
        BeginScissorMode(static_cast<int>(_scrollView.x), static_cast<int>(_scrollView.y), static_cast<int>(_scrollView.width), static_cast<int>(_scrollView.height));
        GuiScrollPanel(_bounds, nullptr, _contentView, &_scrollPos, &_scrollView);
    }

    void UiScrollPanel::update(float deltaTime)
    {
        updateScrollView();
        setScrollOffset(_scrollPos);
    }

    void UiScrollPanel::onFrameEnd(const float deltaTime)
    {
        EndScissorMode();
    }

    void UiScrollPanel::calculateRectForScroll(UiElement *element)
    {
        const auto &elementPosition = element->getPosition();
        const auto &elementSize = element->getSize();
        _contentSize = Vector2(elementPosition.x + elementSize.x, elementPosition.y + elementSize.y);
        setDirty();
    }

    bool UiScrollPanel::tryDeleteSelf()
    {
        UiPool::scrollPanelPool.release(*this);
        return true;
    }

    void UiScrollPanel::updateScrollView()
    {
        const auto &bounds = getBounds();
        _contentView.x = bounds.x;
        _contentView.y = bounds.y;
        _contentView.height = _contentSize.y;
        _contentView.width = _contentSize.x;
        if (_contentView.height < bounds.height)
        {
            _contentView.height = bounds.height - 2 * static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) - static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH));
        }

        if (_contentView.width < bounds.width)
        {
            _contentView.width = bounds.width - 2 * static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) - static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH));
        }
    }
} // BreadEditor
