#include "uiColor.h"

#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiColor::UiColor() = default;

    UiColor &UiColor::setup(const std::string_view &id, const Color color)
    {
        UiElement::setup(id);
        _value = color;
        return *this;
    }

    UiColor &UiColor::setup(const std::string_view &id, UiElement *parentElement, const Color color)
    {
        UiElement::setup(id, parentElement);
        _value = color;
        return *this;
    }

    UiColor &UiColor::setup(const std::string_view &id, UiElement *parentElement, const std::function<Color()> &getFunc)
    {
        UiElement::setup(id, parentElement);
        _getFunc = getFunc;
        _value = getFunc();
        return *this;
    }

    void UiColor::draw(float deltaTime)
    {
        if (_updateState)
        {
            if (_getFunc != nullptr)
            {
                _value = _getFunc();
            }
        }

        _internalValue = &_value;

        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(_value));
        GuiPanel(_bounds, nullptr);
        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0xf5f5f5ff);
    }

    void UiColor::update(float deltaTime)
    {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Engine::isCollisionPointRec(GetMousePosition(), _bounds))
        {
            onSelectorRequested.invoke(_value);
        }
    }

    void UiColor::dispose()
    {
        onValueChanged.unsubscribeAll();
        onSelectorRequested.unsubscribeAll();
        _value = WHITE;
        _getFunc = nullptr;
        _internalValue = nullptr;
        _updateState = true;

        UiElement::dispose();
    }

    void UiColor::setColor(const Color color, const bool withNotification)
    {
        _value = color;
        if (!withNotification) return;
        onValueChanged.invoke(color);
    }

    void UiColor::setUpdateState(const bool state)
    {
        _updateState = state;
    }

    bool UiColor::tryDeleteSelf()
    {
        UiPool::colorPool.release(*this);
        return true;
    }
} // BreadEditor
