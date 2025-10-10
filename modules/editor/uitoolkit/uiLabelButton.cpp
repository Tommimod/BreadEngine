#include "uiLabelButton.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiLabelButton::UiLabelButton() = default;

    UiLabelButton &UiLabelButton::setup(const std::string &id, const std::string &newText)
    {
        this->text = newText;
        return dynamic_cast<UiLabelButton &>(UiElement::setup(id));
    }

    UiLabelButton &UiLabelButton::setup(const std::string &id, UiElement *parentElement, const std::string &newText)
    {
        this->text = newText;
        return dynamic_cast<UiLabelButton &>(UiElement::setup(id, parentElement));
    }

    UiLabelButton::~UiLabelButton() = default;

    void UiLabelButton::draw(const float deltaTime)
    {
        if (GuiLabelButton(bounds, text.c_str()))
        {
            onClick.invoke(text);
        }

        UiElement::draw(deltaTime);
    }

    void UiLabelButton::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiLabelButton::deleteSelf()
    {
        UiPool::labelButtonPool.release(*this);
    }
} // namespace BreadEditor
