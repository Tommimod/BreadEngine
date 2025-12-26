#include "UiButton.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiButton::UiButton() = default;

    UiButton &UiButton::setup(const std::string &id, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiButton &>(UiElement::setup(id));
    }

    UiButton &UiButton::setup(const std::string &id, UiElement *parentElement, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiButton &>(UiElement::setup(id, parentElement));
    }

    UiButton::~UiButton() = default;

    void UiButton::dispose()
    {
        onClick.unsubscribeAll();
        UiElement::dispose();
    }

    void UiButton::draw(const float deltaTime)
    {
        if (!isActive) return;

        GuiSetState(_state);
        if (GuiButton(_bounds, _text.c_str()))
        {
            onClick.invoke(this);
        }

        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiButton::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    bool UiButton::tryDeleteSelf()
    {
        UiPool::buttonPool.release(*this);
        return true;
    }
} // namespace BreadEditor
