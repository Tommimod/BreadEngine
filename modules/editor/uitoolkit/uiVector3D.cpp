#include "uiVector3D.h"
#include "uiPool.h"

namespace BreadEditor {
    UiVector3D::~UiVector3D()
    {
        for (const auto &field: _fields)
        {
            field->onValueChangedWithSender.unsubscribeAll();
        }
    }

    UiVector3D *UiVector3D::setup(const std::string &id, UiElement *parentElement, const Vector3 initialValue)
    {
        UiElement::setup(id, parentElement);
        _value = initialValue;
        _names[0] = "X";
        _names[1] = "Y";
        _names[2] = "Z";
        return this;
    }

    UiVector3D *UiVector3D::setup(const std::string &id, UiElement *parentElement, const Vector3 initialValue, const std::string_view xName, const std::string_view yName, const std::string_view zName)
    {
        UiElement::setup(id, parentElement);
        _value = initialValue;
        _names[0] = xName;
        _names[1] = yName;
        _names[2] = zName;
        return this;
    }

    void UiVector3D::draw(float deltaTime)
    {
        GuiSetState(_state);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiVector3D::update(float deltaTime)
    {
        createFields();
        UiElement::update(deltaTime);
    }

    bool UiVector3D::tryDeleteSelf()
    {
        UiPool::vector3DPool.release(*this);
        return true;
    }

    void UiVector3D::createFields()
    {
        if (_fields[0] != nullptr)
        {
            return;
        }

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

            const auto field = &UiPool::numberBoxPool.get().setup(TextFormat("Vector3D%s_Field%i", id.c_str(), i), this, _names[i], value);
            _fields[i] = field;
            field->onValueChangedWithSender.subscribe([this](const float floatValue, UiNumberBox *thisField)
            {
                setValue(thisField, floatValue);
            });

            if (const auto indexOf = static_cast<float>(i); indexOf > 0)
            {
                lastSizeX = _fields[i - 1]->getSize().x + _fields[i - 1]->getPosition().x;
            }

            field->setAnchor(UI_LEFT_TOP);
            field->setPivot({0, 0});
            field->setSizePercentPermanent({.15f, 1});
            field->setPosition({lastSizeX + 15, 0});
            field->setSizeMax({50, 0});
        }
    }

    void UiVector3D::setValue(UiNumberBox *numberBox, const float value)
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

        onChanged.invoke(_value);
    }
} // BreadEditor
