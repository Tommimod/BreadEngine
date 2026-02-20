#include "uiTextBox.h"

#include <any>
#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiTextBox::UiTextBox() = default;

    UiTextBox &UiTextBox::setup(const std::string &id, const std::string &defaultText, const int defaultTextSize, const bool defaultEditMode)
    {
        _internalText = defaultText;
        _text = _internalText.data();
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, const std::string &defaultText, const int defaultTextSize, const bool defaultEditMode)
    {
        _internalText = defaultText;
        _text = _internalText.data();
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, std::function<std::string()> getFunc, const int defaultTextSize, const bool defaultEditMode)
    {
        _getFunc = std::move(getFunc);
        _internalText = _getFunc();
        _text = _internalText.data();
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox::~UiTextBox()
    {
        delete _text;
    }

    void UiTextBox::dispose()
    {
        onValueChanged.unsubscribeAll();
        onValueChangedWithSender.unsubscribeAll();
        _text = nullptr;
        _getFunc = nullptr;
        UiElement::dispose();
    }

    void UiTextBox::draw(const float deltaTime)
    {
        GuiTextBox(_bounds, _text, _textSize, _editMode);
    }

    void UiTextBox::update(const float deltaTime)
    {
        if (_state == STATE_DISABLED)
        {
            return;
        }

        if (Engine::isCollisionPointRec(GetMousePosition(), _bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _editMode = true;
        }
        else if (_editMode && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _editMode = false;
            onValueChanged.invoke(_text);
            onValueChangedWithSender.invoke(_text, this);
        }

        if (_editMode && IsKeyDown(KEY_ENTER))
        {
            _editMode = false;
            onValueChanged.invoke(_text);
            onValueChangedWithSender.invoke(_text, this);
        }

        if (!_editMode && _getFunc != nullptr)
        {
            _internalText = _getFunc();
        }
    }

    void UiTextBox::setText(const std::string &newText)
    {
        _internalText = newText;
    }

    bool UiTextBox::tryDeleteSelf()
    {
        UiPool::textBoxPool.release(*this);
        return true;
    }
} // BreadEditor
