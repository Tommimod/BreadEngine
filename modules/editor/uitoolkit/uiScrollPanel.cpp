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

    bool UiScrollPanel::tryDeleteSelf()
    {
        UiPool::scrollPanelPool.release(*this);
        return true;
    }

    void UiScrollPanel::updateScrollView()
    {
        _contentView.x = getBounds().x;
        _contentView.y = getBounds().y;
        _contentView.height = _contentSize.y;
        _contentView.width = _contentSize.x;
        if (_contentView.height < getBounds().height)
        {
            _contentView.height = getBounds().height - static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) - static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH)) - 15;
        }

        if (_contentView.width < getBounds().width)
        {
            _contentView.width = getBounds().width - static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) - static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH)) - 1;
        }
    }

    void UiScrollPanel::calculateRectForScroll(UiElement *element)
    {
        _contentSize = {0, 0};

        const auto &elementPosition = element->getPosition();
        const auto &elementSize = element->getSize();
        const auto size = Vector2(elementPosition.x + elementSize.x, elementPosition.y + elementSize.y);
        if (_contentSize.x == 0)
        {
            _contentSize.x = size.x;
        }
        else
        {
            if (_contentSize.x < size.x) _contentSize.x = size.x;
        }

        if (_contentSize.y == 0)
        {
            _contentSize.y = size.y;
        }
        else
        {
            if (_contentSize.y < size.y) _contentSize.y = size.y;
        }

        setDirty();
    }
} // BreadEditor
