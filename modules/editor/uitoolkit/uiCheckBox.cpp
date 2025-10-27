#include "uiCheckBox.h"
#include "uiPool.h"

namespace BreadEditor {
    UiCheckBox::UiCheckBox() = default;

    UiCheckBox &UiCheckBox::setup(const std::string &id, bool checked, std::string checkBoxText)
    {
        this->text = checkBoxText;
        this->internalChecked = checked;
        this->externalChecked = nullptr;
        UiElement::setup(id);
        return *this;
    }

    UiCheckBox &UiCheckBox::setup(const std::string &id, UiElement *parentElement, bool checked, std::string checkBoxText)
    {
        this->text = checkBoxText;
        this->internalChecked = checked;
        this->externalChecked = nullptr;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiCheckBox::~UiCheckBox()
    {
    }

    void UiCheckBox::draw(float deltaTime)
    {
        GuiSetState(state);
        auto lastState = internalChecked;
        externalChecked = &internalChecked;
        GuiCheckBox(bounds, text.c_str(), &internalChecked);
        if (externalChecked != nullptr && lastState != *externalChecked)
        {
            onStateChanged.invoke(internalChecked);
        }
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiCheckBox::update(float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiCheckBox::setChecked(bool isChecked)
    {
        internalChecked = isChecked;
    }

    void UiCheckBox::deleteSelf()
    {
        UiPool::checkBoxPool.release(*this);
    }
} // BreadEditor
