#include <format>
#include "UiNumberBox.h"
#include "uiPool.h"

namespace BreadEditor {
    UiNumberBox::UiNumberBox() = default;

    UiNumberBox::~UiNumberBox() = default;

    void UiNumberBox::dispose()
    {
        _floatValue = 0;
        _intValue = 0;
        _editMode = false;
        _intMode = false;
        _label = "";
        _floatLabel.clear();
        _getFunc = nullptr;
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
        _label = label;
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
        _label = label;
        _intValue = defaultValue;
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%i", _intValue);
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiNumberBox &UiNumberBox::setup(const std::string &id, UiElement *parentElement, const std::string &label, std::function<std::variant<int, float, long>()> getFunc, const int defaultTextSize, const bool defaultEditMode)
    {
        _intMode = true;
        _label = label;
        _getFunc = std::move(getFunc);
        _textSize = defaultTextSize;
        snprintf(_valueText, sizeof(_valueText), "%i", _intValue);
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiNumberBox::draw(const float deltaTime)
    {
        if (_intMode)
        {
            if (GuiValueBox(_bounds, _label.c_str(), &_intValue, INT_MIN,INT_MAX, _editMode))
            {
                _editMode = !_editMode;
                if (!_editMode)
                {
                    onValueChanged.invoke(static_cast<float>(_intValue));
                    onValueChangedWithSender.invoke(static_cast<float>(_intValue), this);
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
    }

    void UiNumberBox::update(const float deltaTime)
    {
        if (!_editMode && _getFunc != nullptr)
        {
            if (_intMode)
            {
                _intValue = std::get<int>(_getFunc());
            }
            else
            {
                _floatValue = std::get<float>(_getFunc());
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
