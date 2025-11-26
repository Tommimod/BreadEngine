#include "uiVector3D.h"
#include "uiPool.h"

namespace BreadEditor {
    UiVector3D::~UiVector3D()
    {
        for (const auto &field: _fields)
        {
            field->onTextChangedWithSender.unsubscribeAll();
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
        UiElement::draw(deltaTime);
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
            std::string value;
            if (i == 0)
            {
                value = std::to_string(_value.x);
            }
            else if (i == 1)
            {
                value = std::to_string(_value.y);
            }
            else if (i == 2)
            {
                value = std::to_string(_value.z);
            }

            value.erase(value.find_last_not_of('0') + 1, std::string::npos);
            value.erase(value.find_last_not_of('.') + 1, std::string::npos);

            const auto field = &UiPool::textBoxPool.get().setup(TextFormat("Vector3D%s_Field%i", id.c_str(), i), this, value);
            _fields[i] = field;
            field->onTextChangedWithSender.subscribe([this](const char *textValue, UiTextBox *thisField)
            {
                setValue(thisField, static_cast<float>(strtod(textValue, nullptr)));
            });

            const auto indexOf = static_cast<float>(i);
            if (indexOf > 0)
            {
                lastSizeX = _fields[i - 1]->getSize().x + _fields[i - 1]->getPosition().x;
            }

            const auto label = UiPool::labelPool.get().setup(TextFormat("Vector3D%s_Label", id.c_str(), this), this, _names[i]);
            label->setAnchor(UI_LEFT_TOP);
            label->setPivot({0, 0});
            label->setPosition({5 + lastSizeX * indexOf, 0});
            label->setSize({10, getSize().y});
            label->computeBounds();

            field->setAnchor(UI_LEFT_TOP);
            field->setPivot({0, 0});
            field->setSizePercentPermanent({.15f, 1});
            field->setPosition({label->getBounds().width + label->getPosition().x + 5, 0});
            field->computeBounds();
        }
    }

    void UiVector3D::setValue(UiTextBox *textBox, const float value)
    {
        const auto it = ranges::find(_fields, textBox);
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
