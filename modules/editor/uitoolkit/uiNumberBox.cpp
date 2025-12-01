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
        UiElement::dispose();
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, const std::string &label, float defaultValue, const int defaultTextSize, const bool defaultEditMode)
    {
        _label = label.c_str();
        _floatValue = defaultValue;
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%.4f", _floatValue);
        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, UiElement *parentElement, const std::string &label, float defaultValue, const int defaultTextSize, const bool defaultEditMode)
    {
        _label = label.c_str();
        _floatValue = defaultValue;
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%.4f", _floatValue);
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
            if (GuiValueBoxFloat(_bounds, _label, _valueText, &_floatValue, _editMode))
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
    }

    void UiNumberBox::setValue(const float value)
    {
        _floatValue = value;
    }

    bool UiNumberBox::tryDeleteSelf()
    {
        UiPool::numberBoxPool.release(*this);
        return true;
    }
} // BreadEditor
