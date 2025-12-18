#include <format>
#include "UiNumberBox.h"
#include "uiPool.h"

namespace BreadEditor {
    UiNumberBox::UiNumberBox() = default;

    UiNumberBox::~UiNumberBox()
    = default;

    void UiNumberBox::dispose()
    {
        _floatValue = 0;
        _intValue = 0;
        _editMode = false;
        _intMode = false;
        _label = nullptr;
        _floatLabel.clear();
        onValueChanged.unsubscribeAll();
        onValueChangedWithSender.unsubscribeAll();

        UiElement::dispose();
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, const std::string &label, const float defaultValue, const int defaultTextSize, const bool defaultEditMode)
    {
        _floatLabel = label;
        _floatValue = defaultValue;
        _textSize = defaultTextSize;
        if (defaultValue == 0.0000f)
        {
            snprintf(_valueText, sizeof(_valueText), "%.0f", _floatValue);
        }
        else
        {
            snprintf(_valueText, sizeof(_valueText), "%.1f", _floatValue);
        }

        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, UiElement *parentElement, const std::string &label, const float defaultValue, const int defaultTextSize, const bool defaultEditMode)
    {
        _floatLabel = label;
        _floatValue = defaultValue;
        _textSize = defaultTextSize;
        if (defaultValue == 0.0000f)
        {
            snprintf(_valueText, sizeof(_valueText), "%.0f", _floatValue);
        }
        else
        {
            snprintf(_valueText, sizeof(_valueText), "%.1f", _floatValue);
        }

        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, const std::string &label, const int defaultValue, const int defaultTextSize, const bool defaultEditMode)
    {
        _intMode = true;
        _label = label.c_str();
        _intValue = defaultValue;
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%i", _intValue);
        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, UiElement *parentElement, const std::string &label, const int defaultValue, const int defaultTextSize, const bool defaultEditMode)
    {
        _intMode = true;
        _label = label.c_str();
        _intValue = defaultValue;
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%i", _intValue);
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, UiElement *parentElement, const std::string &label, UiComponent::PropWithComponent dynamicValue, int defaultTextSize, bool defaultEditMode)
    {
        _intMode = true;
        _label = label.c_str();
        _dynamicValue = std::move(dynamicValue);
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%i", _intValue);
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiNumberBox::draw(const float deltaTime)
    {
        GuiSetState(_state);

        if (_intMode)
        {
            if (GuiValueBox(_bounds, _label, &_intValue, INT_MIN,INT_MAX, _editMode))
            {
                _editMode = !_editMode;
                if (!_editMode)
                {
                    onValueChanged.invoke(_intValue);
                    onValueChangedWithSender.invoke(_intValue, this);
                }
            }
        }
        else
        {
            if (GuiValueBoxFloat(_bounds, _floatLabel.c_str(), _valueText, &_floatValue, _editMode))
            {
                _editMode = !_editMode;
                if (!_editMode)
                {
                    onValueChanged.invoke(_floatValue);
                    onValueChangedWithSender.invoke(_floatValue, this);
                }
            }
        }

        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiNumberBox::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
        if (!_editMode && _dynamicValue.component != nullptr)
        {
            if (_intMode)
            {
                _intValue = get<int>(_dynamicValue.property->get(_dynamicValue.component));
            }
            else
            {
                _floatValue = get<float>(_dynamicValue.property->get(_dynamicValue.component));
            }
        }
    }

    void UiNumberBox::setValue(const int value)
    {
        if (_editMode)
        {
            return;
        }

        _intValue = value;
        snprintf(_valueText, sizeof(_valueText), "%i", _intValue);
    }

    void UiNumberBox::setValue(const float value)
    {
        if (_editMode)
        {
            return;
        }

        _floatValue = value;
        if (_floatValue == 0.0000f)
        {
            snprintf(_valueText, sizeof(_valueText), "%.0f", _floatValue);
        }
        else
        {
            snprintf(_valueText, sizeof(_valueText), "%.1f", _floatValue);
        }
    }

    bool UiNumberBox::tryDeleteSelf()
    {
        UiPool::numberBoxPool.release(*this);
        return true;
    }
} // BreadEditor
