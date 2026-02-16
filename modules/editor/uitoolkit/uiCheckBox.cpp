#include "uiCheckBox.h"

#include <any>
#include <utility>
#include "uiPool.h"

namespace BreadEditor {
    UiCheckBox::UiCheckBox() = default;

    UiCheckBox &UiCheckBox::setup(const std::string &id, std::string checkBoxText, const bool checked)
    {
        this->_text = std::move(checkBoxText);
        this->_internalChecked = checked;
        this->_externalChecked = nullptr;
        UiElement::setup(id);
        return *this;
    }

    UiCheckBox &UiCheckBox::setup(const std::string &id, UiElement *parentElement, std::string checkBoxText, const bool checked)
    {
        this->_text = std::move(checkBoxText);
        this->_internalChecked = checked;
        this->_externalChecked = nullptr;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiCheckBox &UiCheckBox::setup(const std::string &id, UiElement *parentElement, std::string checkBoxText, UiInspector::PropsOfStruct dynamicValue)
    {
        this->_text = std::move(checkBoxText);
        _dynamicValue = std::move(dynamicValue);
        this->_internalChecked = std::any_cast<bool>(_dynamicValue.property->get(_dynamicValue.inspectorStruct));
        this->_externalChecked = nullptr;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiCheckBox::~UiCheckBox()
    {
        delete _externalChecked;
    }

    void UiCheckBox::dispose()
    {
        _internalChecked = false;
        _externalChecked = nullptr;
        _text.clear();
        onValueChanged.unsubscribeAll();
        UiElement::dispose();
    }

    void UiCheckBox::draw(const float deltaTime)
    {
        const auto lastState = _internalChecked;
        _externalChecked = &_internalChecked;
        GuiCheckBox(_bounds, _text.c_str(), &_internalChecked);
        if (_externalChecked != nullptr && lastState != *_externalChecked)
        {
            onValueChanged.invoke(_internalChecked);
        }
    }

    void UiCheckBox::update(const float deltaTime)
    {
        if (_dynamicValue.inspectorStruct == nullptr)
        {
            return;
        }

        this->_internalChecked = std::any_cast<bool>(_dynamicValue.property->get(_dynamicValue.inspectorStruct));
    }

    void UiCheckBox::setChecked(const bool isChecked)
    {
        _internalChecked = isChecked;
    }

    bool UiCheckBox::tryDeleteSelf()
    {
        UiPool::checkBoxPool.release(*this);
        return true;
    }
} // BreadEditor
