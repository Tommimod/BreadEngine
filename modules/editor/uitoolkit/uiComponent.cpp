#include "uiComponent.h"
#include "uiPool.h"
using namespace BreadEngine;

namespace BreadEditor {
    UiComponent::UiComponent() = default;

    UiComponent &UiComponent::setup(const std::string &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiComponent::~UiComponent() = default;

    void UiComponent::trackComponent(Component *component)
    {
        constexpr std::string transformName = "Transform";

        _component = component;
        _componentName = component->getTypeName();
        _isTransform = _componentName == transformName;
        _properties = component->getInspectedProperties();

        for (int i = 0; i < static_cast<int>(_properties.size()); i++)
        {
            const auto &property = _properties[i];
            auto propValue = property.get(component);
            constexpr float horOffset = 5;
            constexpr float heightSize = 17;
            const auto verOffset = horOffset * static_cast<float>(i) + 30 + heightSize * static_cast<float>(i);
            setSize({0, verOffset + heightSize + horOffset});

            const auto propName = UiPool::labelPool.get().setup(TextFormat("PropName %s", property.name.c_str()), this, property.name);
            propName->setAnchor(UI_LEFT_TOP);
            propName->setSize({70, heightSize});
            propName->setPosition({horOffset, verOffset});
            UiElement *createdElement = nullptr;

            auto isSingleField = true;
            if (property.type == PropertyType::COMPONENT)
            {
                //TODO
            }
            else if (property.type == PropertyType::INT)
            {
                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, "", propValue.emplace<int>());
                const auto element = dynamic_cast<UiNumberBox *>(createdElement);
                element->onValueChanged.subscribe([component, property](const int &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::FLOAT)
            {
                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, "", propValue.emplace<float>());
                const auto element = dynamic_cast<UiNumberBox *>(createdElement);
                element->onValueChanged.subscribe([component, property](const float &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::LONG)
            {
                float val = propValue.emplace<long>();
                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, "", val);
                const auto element = dynamic_cast<UiNumberBox *>(createdElement);
                element->onValueChanged.subscribe([component, property](const long &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::BOOL)
            {
                createdElement = &UiPool::checkBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), propValue.emplace<bool>(), "");
                const auto element = dynamic_cast<UiCheckBox *>(createdElement);
                element->onValueChanged.subscribe([component, property](const bool &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::STRING)
            {
                createdElement = &UiPool::textBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, propValue.emplace<std::string>());
                const auto element = dynamic_cast<UiTextBox *>(createdElement);
                element->onValueChanged.subscribe([component, property](const std::string &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::VECTOR2)
            {
                isSingleField = false;
                createdElement = UiPool::vector2DPool.get().setup(TextFormat("Vector2D %s", property.name.c_str()), this, propValue.emplace<Vector2>());
                const auto element = dynamic_cast<UiVector2D *>(createdElement);
                element->onChanged.subscribe([component, property](const Vector2 &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::VECTOR3)
            {
                isSingleField = false;
                createdElement = UiPool::vector3DPool.get().setup(TextFormat("Vector3D %s", property.name.c_str()), this, propValue.emplace<Vector3>());
                const auto element = dynamic_cast<UiVector3D *>(createdElement);
                element->onChanged.subscribe([component, property](const Vector3 &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::VECTOR4)
            {
                isSingleField = false;
                createdElement = UiPool::vector4DPool.get().setup(TextFormat("Vector4D %s", property.name.c_str()), this, propValue.emplace<Vector4>());
                const auto element = dynamic_cast<UiVector4D *>(createdElement);
                element->onChanged.subscribe([component, property](const Vector4 &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::COLOR)
            {
                //TODO
            }
            else if (property.type == PropertyType::ENUM)
            {
                //TODO
            }

            if (createdElement)
            {
                const auto propNameWidth = propName->getSize().x + propName->getPosition().x;
                createdElement->setAnchor(UI_LEFT_TOP);
                createdElement->setSize({getSize().x, heightSize});
                auto sizeInPercent = isSingleField ? .3f : 1;
                createdElement->setSizePercentPermanent({sizeInPercent, -1});
                createdElement->setPosition({propNameWidth, verOffset});
            }
        }
    }

    void UiComponent::draw(const float deltaTime)
    {
        if (_isTransform)
        {
            GuiPanel(_bounds, _componentName.c_str());
        }
        else
        {
            if (GuiWindowBox(_bounds, _componentName.c_str()))
            {
                onDelete.invoke(typeid(_component));
            }
        }

        UiElement::draw(deltaTime);
    }

    void UiComponent::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    Component *UiComponent::getTrackedComponent() const
    {
        return _component;
    }

    bool UiComponent::tryDeleteSelf()
    {
        UiPool::componentPool.release(*this);
        return true;
    }
}
