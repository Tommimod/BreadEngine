#include "uiCheckBox.h"

#include <utility>
#include "uiPool.h"

namespace BreadEditor {
    UiCheckBox::UiCheckBox() = default;

    UiCheckBox &UiCheckBox::setup(const std::string &id, bool checked, std::string checkBoxText)
    {
        this->_text = std::move(checkBoxText);
        this->_internalChecked = checked;
        this->_externalChecked = nullptr;
        UiElement::setup(id);
        return *this;
    }

    UiCheckBox &UiCheckBox::setup(const std::string &id, UiElement *parentElement, bool checked, std::string checkBoxText)
    {
        this->_text = std::move(checkBoxText);
        this->_internalChecked = checked;
        this->_externalChecked = nullptr;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiCheckBox::~UiCheckBox()
    = default;

    void UiCheckBox::draw(float deltaTime)
    {
        GuiSetState(_state);
        auto lastState = _internalChecked;
        _externalChecked = &_internalChecked;
        GuiCheckBox(_bounds, _text.c_str(), &_internalChecked);
        if (_externalChecked != nullptr && lastState != *_externalChecked)
        {
            onStateChanged.invoke(_internalChecked);
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
        _internalChecked = isChecked;
    }

    bool UiCheckBox::tryDeleteSelf()
    {
        UiPool::checkBoxPool.release(*this);
        return true;
    }
} // BreadEditor
