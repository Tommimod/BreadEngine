#include "uiLabelButton.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiLabelButton::UiLabelButton() = default;

    UiLabelButton &UiLabelButton::setup(const std::string &id, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiLabelButton &>(UiElement::setup(id));
    }

    UiLabelButton &UiLabelButton::setup(const std::string &id, UiElement *parentElement, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiLabelButton &>(UiElement::setup(id, parentElement));
    }

    UiLabelButton::~UiLabelButton() = default;

    void UiLabelButton::draw(const float deltaTime)
    {
        GuiSetState(_state);
        if (GuiLabelButton(_bounds, _text.c_str()))
        {
            onClick.invoke(_text);
        }

        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiLabelButton::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    bool UiLabelButton::tryDeleteSelf()
    {
        UiPool::labelButtonPool.release(*this);
        return true;
    }
} // namespace BreadEditor
