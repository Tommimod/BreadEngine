#include "uiVector4D.h"
#include "uiPool.h"

namespace BreadEditor {
    void UiVector4D::dispose()
    {
        for (auto &_field: _fields)
        {
            if (_field == nullptr) continue;

            _field->onValueChangedWithSender.unsubscribeAll();
            _field = nullptr;
        }

        _getFunc = nullptr;
        onChanged.unsubscribeAll();
        UiElement::dispose();
    }

    UiVector4D *UiVector4D::setup(const std::string &id, UiElement *parentElement, const Vector4 initialValue)
    {
        UiElement::setup(id, parentElement);
        _value = initialValue;
        _names[0] = "X";
        _names[1] = "Y";
        _names[2] = "Z";
        _names[3] = "W";
        return this;
    }

    UiVector4D *UiVector4D::setup(const std::string &id, UiElement *parentElement, const Vector4 initialValue, const std::string_view xName, const std::string_view yName, const std::string_view zName, const std::string_view wName)
    {
        UiElement::setup(id, parentElement);
        _value = initialValue;
        _names[0] = xName;
        _names[1] = yName;
        _names[2] = zName;
        _names[3] = wName;
        return this;
    }

    UiVector4D *UiVector4D::setup(const std::string &id, UiElement *parentElement, std::function<Vector4()> getFunc)
    {
        UiElement::setup(id, parentElement);
        _getFunc = std::move(getFunc);
        _value = _getFunc();
        _names[0] = "X";
        _names[1] = "Y";
        _names[2] = "Z";
        _names[3] = "W";
        return this;
    }

    UiVector4D *UiVector4D::setup(const std::string &id, UiElement *parentElement, std::function<Vector4()> getFunc, const std::string_view xName, const std::string_view yName, const std::string_view zName, const std::string_view wName)
    {
        UiElement::setup(id, parentElement);
        _getFunc = std::move(getFunc);
        _value = _getFunc();
        _names[0] = xName;
        _names[1] = yName;
        _names[2] = zName;
        _names[3] = wName;
        return this;
    }

    void UiVector4D::draw(float deltaTime)
    {
    }

    void UiVector4D::update(float deltaTime)
    {
        createFields();
    }

    bool UiVector4D::tryDeleteSelf()
    {
        UiPool::vector4DPool.release(*this);
        return true;
    }

    void UiVector4D::createFields()
    {
        if (_fields[0] != nullptr)
        {
            if (_getFunc != nullptr)
            {
                _value = _getFunc();
                _fields[0]->setValue(_value.x);
                _fields[1]->setValue(_value.y);
                _fields[2]->setValue(_value.z);
                _fields[3]->setValue(_value.w);
            }

            return;
        }

        computeBounds();
        float lastSizeX = 0;
        for (size_t i = 0; i < _fields.size(); i++)
        {
            float value = 0;
            if (i == 0)
            {
                value = _value.x;
            }
            else if (i == 1)
            {
                value = _value.y;
            }
            else if (i == 2)
            {
                value = _value.z;
            }
            else if (i == 3)
            {
                value = _value.w;
            }

            const auto field = &UiPool::numberBoxPool.get().setup(TextFormat("Vector4D%s_Field%i", id.c_str(), i), this, _names[i], value);
            _fields[i] = field;
            field->onValueChangedWithSender.subscribe([this](const float floatValue, UiNumberBox *thisField)
            {
                setValue(thisField, floatValue);
            });

            if (const auto indexOf = static_cast<float>(i); indexOf > 0)
            {
                lastSizeX = _fields[i - 1]->getSize().x + _fields[i - 1]->getPosition().x + 25;
            }
            else
            {
                lastSizeX = 15;
            }

            field->setAnchor(UI_LEFT_TOP);
            field->setPivot({0, 0});
            field->setSizePercentPermanent({.12f, 1});
            field->setPosition({lastSizeX, 0});
            field->setSizeMax({100, 0});
        }
    }

    void UiVector4D::setValue(UiNumberBox *numberBox, const float value)
    {
        const auto it = ranges::find(_fields, numberBox);
        if (const auto index_of_x = std::distance(std::begin(_fields), it); index_of_x == 0)
        {
            _value.x = value;
        }
        else if (index_of_x == 1)
        {
            _value.y = value;
        }
        else if (index_of_x == 2)
        {
            _value.z = value;
        }
        else if (index_of_x == 3)
        {
            _value.w = value;
        }

        onChanged.invoke(_value);
    }
} // BreadEditor
