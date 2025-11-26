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
            constexpr float heightSize = 15;
            const auto verOffset = horOffset * static_cast<float>(i) + 30 + heightSize * static_cast<float>(i);
            setSize({getSize().x, verOffset + heightSize + horOffset});

            const auto propName = UiPool::labelPool.get().setup(TextFormat("PropName %s", property.name.c_str()), this, property.name);
            propName->setAnchor(UI_LEFT_TOP);
            propName->setSize({70, heightSize});
            propName->setPosition({horOffset, verOffset});
            UiElement *createdElement = nullptr;

            if (property.type == PropertyType::COMPONENT)
            {
            }
            else if (property.type == PropertyType::INT)
            {
            }
            else if (property.type == PropertyType::FLOAT)
            {
            }
            else if (property.type == PropertyType::LONG)
            {
            }
            else if (property.type == PropertyType::BOOL)
            {
            }
            else if (property.type == PropertyType::STRING)
            {
            }
            else if (property.type == PropertyType::VECTOR2)
            {
                createdElement = UiPool::vector2DPool.get().setup(TextFormat("Vector2D %s", property.name.c_str()), this, propValue.emplace<Vector2>());
                const auto element = dynamic_cast<UiVector2D *>(createdElement);
                element->onChanged.subscribe([component, property](const Vector2 &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::VECTOR3)
            {
                createdElement = UiPool::vector3DPool.get().setup(TextFormat("Vector3D %s", property.name.c_str()), this, propValue.emplace<Vector3>());
                const auto element = dynamic_cast<UiVector3D *>(createdElement);
                element->onChanged.subscribe([component, property](const Vector3 &value)
                {
                    property.set(component, value);
                });
            }
            else if (property.type == PropertyType::VECTOR4)
            {
            }
            else if (property.type == PropertyType::COLOR)
            {
            }
            else if (property.type == PropertyType::ENUM)
            {
            }

            if (createdElement)
            {
                const auto propNameWidth = propName->getSize().x + propName->getPosition().x;
                createdElement->setAnchor(UI_LEFT_TOP);
                createdElement->setSize({getSize().x, heightSize});
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
} // BreadEditor
