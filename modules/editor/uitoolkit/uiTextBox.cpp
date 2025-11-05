#include "uiTextBox.h"

#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiTextBox::UiTextBox() = default;

    UiTextBox &UiTextBox::setup(const std::string &id, std::string defaultText, int defaultTextSize, bool defaultEditMode)
    {
        _internalText = defaultText;
        _text = _internalText.data();
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, std::string defaultText, int defaultTextSize, bool defaultEditMode)
    {
        _internalText = defaultText;
        _text = _internalText.data();
        _textSize = defaultTextSize;
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox::~UiTextBox()
    {
    }

    void UiTextBox::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiTextBox(_bounds, _text, _textSize, _editMode);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiTextBox::update(float deltaTime)
    {
        UiElement::update(deltaTime);
        if (_state == STATE_DISABLED)
        {
            return;
        }

        if (Engine::IsCollisionPointRec(GetMousePosition(), _bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _editMode = true;
        }
        else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _editMode = false;
            onTextChanged.invoke(_text);
        }

        if (_editMode && IsKeyDown(KEY_ENTER))
        {
            _editMode = false;
            onTextChanged.invoke(_text);
        }
    }

    void UiTextBox::setText(std::string newText)
    {
        _internalText = newText;
    }

    bool UiTextBox::tryDeleteSelf()
    {
        UiPool::textBoxPool.release(*this);
        return true;
    }
} // BreadEditor
