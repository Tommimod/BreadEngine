#include "uiTextBox.h"

#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiTextBox::UiTextBox() = default;

    UiTextBox &UiTextBox::setup(const std::string &id, const std::string &defaultText, const int defaultTextSize, const bool defaultEditMode)
    {
        _internalText = defaultText;
        _text = strdup(_internalText.c_str());
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, const std::string &defaultText, const int defaultTextSize, const bool defaultEditMode)
    {
        _internalText = defaultText;
        _text = strdup(_internalText.c_str());
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, std::function<std::string()> getFunc, const int defaultTextSize, const bool defaultEditMode)
    {
        _getFunc = std::move(getFunc);
        _internalText = _getFunc();
        _text = strdup(_internalText.c_str());
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox::~UiTextBox() = default;

    void UiTextBox::dispose()
    {
        onValueChanged.unsubscribeAll();
        onValueChangedWithSender.unsubscribeAll();
        free(_text);
        _text = nullptr;
        _getFunc = nullptr;
        _internalText.clear();
        _textSize = 16;
        _editMode = false;
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
        free(_text);
        _internalText = newText;
        _text = strdup(_internalText.c_str());
    }

    bool UiTextBox::tryDeleteSelf()
    {
        UiPool::textBoxPool.release(*this);
        return true;
    }
} // BreadEditor
