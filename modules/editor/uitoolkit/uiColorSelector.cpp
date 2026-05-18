#include "uiColorSelector.h"

#include "editor.h"
#include "uiPool.h"
#include "utils/colorUtils.h"

namespace BreadEditor {
    UiColorSelector &UiColorSelector::setup(const std::string_view &id, const Color color)
    {
        _color = color;
        _color4 = ColorUtils::ToVector4(color);
        _internalColor = &_color;
        _internalValue4 = &_color4;
        _internalAlpha = &_color4.w;
        UiElement::setup(id);
        return *this;
    }

    UiColorSelector &UiColorSelector::setup(const std::string_view &id, UiElement *parentElement, const Color color)
    {
        _color = color;
        _color4 = ColorUtils::ToVector4(color);
        _internalColor = &_color;
        _internalValue4 = &_color4;
        _internalAlpha = &_color4.w;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiColorSelector &UiColorSelector::setup(const std::string_view &id, UiElement *parentElement, const Color color, UiColor *uiColorElement)
    {
        _color = color;
        _color4 = ColorUtils::ToVector4(color);
        _internalColor = &_color;
        _internalValue4 = &_color4;
        _internalAlpha = &_color4.w;
        _uiColorElement = uiColorElement;
        _uiColorElement->setUpdateState(false);
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiColorSelector::draw(float deltaTime)
    {
        if (GuiWindowBox(_bounds, "Color Selector"))
        {
            _parent->destroyChild(this);
            if (_uiColorElement != nullptr)
            {
                _uiColorElement->setUpdateState(true);
                _uiColorElement->setColor(Color(_internalColor->r, _internalColor->g, _internalColor->b, _internalValue4->w), true);
            }
            return;
        }

        Rectangle localBounds = getBounds();
        localBounds.height -= RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT * 2;
        localBounds.y += RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
        GuiColorPicker(localBounds, nullptr, _internalColor);
        localBounds = Rectangle(localBounds.x, localBounds.y + localBounds.height, localBounds.width, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT);
        GuiColorBarAlpha(localBounds, nullptr, _internalAlpha);
    }

    void UiColorSelector::update(float deltaTime)
    {
        if (_uiColorElement == nullptr) return;
        _uiColorElement->setColor(Color(_internalColor->r, _internalColor->g, _internalColor->b, _internalValue4->w), false);
    }

    void UiColorSelector::dispose()
    {
        _internalColor = nullptr;
        _internalValue4 = nullptr;
        _internalAlpha = nullptr;
        _uiColorElement = nullptr;
        _color = WHITE;
        _color4 = Vector4{};
        UiElement::dispose();
    }

    bool UiColorSelector::tryDeleteSelf()
    {
        UiPool::colorSelectorPool.release(*this);
        return true;
    }
} // BreadEditor
