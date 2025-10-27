#include "uiTextBox.h"

#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiTextBox::UiTextBox() = default;

    UiTextBox &UiTextBox::setup(const std::string &id, std::string defaultText, int defaultTextSize, bool defaultEditMode)
    {
        internalText = defaultText;
        text = internalText.data();
        textSize = defaultTextSize;
        editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, std::string defaultText, int defaultTextSize, bool defaultEditMode)
    {
        internalText = defaultText;
        text = internalText.data();
        textSize = defaultTextSize;
        editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox::~UiTextBox()
    {
    }

    void UiTextBox::draw(float deltaTime)
    {
        GuiSetState(state);
        GuiTextBox(bounds, text, textSize, editMode);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiTextBox::update(float deltaTime)
    {
        UiElement::update(deltaTime);
        if (state == STATE_DISABLED)
        {
            return;
        }

        if (Engine::IsCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            editMode = true;
        }
        else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            editMode = false;
            onTextChanged.invoke(text);
        }

        if (editMode && IsKeyDown(KEY_ENTER))
        {
            editMode = false;
            onTextChanged.invoke(text);
        }
    }

    void UiTextBox::setText(std::string newText)
    {
        internalText = newText;
    }

    bool UiTextBox::tryDeleteSelf()
    {
        UiPool::textBoxPool.release(*this);
        return true;
    }
} // BreadEditor
