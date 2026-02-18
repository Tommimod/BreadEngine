#include "uiInspector.h"

#include <thread>

#include "uiPool.h"
using namespace BreadEngine;

namespace BreadEditor {
    UiInspector::UiInspector() = default;

    UiInspector::~UiInspector() = default;

    UiInspector &UiInspector::setup(const std::string &id, UiElement *parentElement, const bool isStatic)
    {
        _isStatic = isStatic;
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiInspector::dispose()
    {
        _inspectorStruct = nullptr;
        onDelete.unsubscribeAll();

        UiElement::dispose();
    }

    void UiInspector::track(InspectorStruct *inspectorStruct)
    {
        cleanUp();
        std::thread workerThread([this, inspectorStruct]
        {
            while (this->getChildCount() > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(32));
            }

            constexpr std::string transformName = "Transform";
            _inspectorStruct = inspectorStruct;
            _componentName = inspectorStruct->getTypeName();
            _isPermanent = _isStatic || _componentName == transformName;
            initializeProperties(inspectorStruct, inspectorStruct->getInspectedProperties());
        });

        workerThread.detach();
    }

    void UiInspector::initializeProperties(InspectorStruct *inspectorStruct, const std::vector<Property> &properties)
    {
        for (int i = 0; i < static_cast<int>(properties.size()); i++)
        {
            constexpr std::string emptyLabel;
            const auto &property = properties[i];
            auto propValue = property.get(inspectorStruct);
            constexpr float horOffset = 5;
            constexpr float heightSize = 17;
            const auto verOffset = horOffset * static_cast<float>(i) + 30 + heightSize * static_cast<float>(i);
            setSize({_localSize.x, verOffset + heightSize + horOffset});

            const auto propName = UiPool::labelPool.get().setup(TextFormat("PropName %s", property.name.c_str()), this, property.name);
            propName->setAnchor(UI_LEFT_TOP);
            propName->setSize({70, heightSize});
            propName->setPosition({horOffset, verOffset});
            UiElement *createdElement = nullptr;

            auto isSingleField = true;
            auto propWithComponent = PropsOfStruct(std::make_unique<Property>(property), inspectorStruct);
            if (property.type == PropertyType::INSPECTOR_STRUCT)
            {
                const auto structPtr = std::any_cast<InspectorStruct *>(property.get(inspectorStruct));
                initializeProperties(structPtr, structPtr->getInspectedProperties());
            }
            else if (property.type == PropertyType::INT)
            {
                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, emptyLabel, std::move(propWithComponent));
                const auto element = dynamic_cast<UiNumberBox *>(createdElement);
                element->onValueChanged.subscribe([inspectorStruct, property](const int &value)
                {
                    property.set(inspectorStruct, value);
                });
            }
            else if (property.type == PropertyType::FLOAT)
            {
                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, emptyLabel, std::move(propWithComponent));
                const auto element = dynamic_cast<UiNumberBox *>(createdElement);
                element->onValueChanged.subscribe([inspectorStruct, property](const float &value)
                {
                    property.set(inspectorStruct, value);
                });
            }
            else if (property.type == PropertyType::LONG)
            {
                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, emptyLabel, std::move(propWithComponent));
                const auto element = dynamic_cast<UiNumberBox *>(createdElement);
                element->onValueChanged.subscribe([inspectorStruct, property](const long &value)
                {
                    property.set(inspectorStruct, value);
                });
            }
            else if (property.type == PropertyType::BOOL)
            {
                createdElement = &UiPool::checkBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, emptyLabel, std::move(propWithComponent));
                const auto element = dynamic_cast<UiCheckBox *>(createdElement);
                element->onValueChanged.subscribe([inspectorStruct, property](const bool &value)
                {
                    property.set(inspectorStruct, value);
                });
                element->setSizeMax({heightSize, heightSize});
            }
            else if (property.type == PropertyType::STRING)
            {
                createdElement = &UiPool::textBoxPool.get().setup(TextFormat("NumberBox %s", property.name.c_str()), this, std::move(propWithComponent));
                const auto element = dynamic_cast<UiTextBox *>(createdElement);
                element->onValueChanged.subscribe([inspectorStruct, property](const std::string &value)
                {
                    property.set(inspectorStruct, value);
                });
            }
            else if (property.type == PropertyType::VECTOR2)
            {
                isSingleField = false;
                createdElement = UiPool::vector2DPool.get().setup(TextFormat("Vector2D %s", property.name.c_str()), this, std::move(propWithComponent));
                const auto element = dynamic_cast<UiVector2D *>(createdElement);
                element->onChanged.subscribe([inspectorStruct, property](const Vector2 &value)
                {
                    property.set(inspectorStruct, value);
                });
            }
            else if (property.type == PropertyType::VECTOR3)
            {
                isSingleField = false;
                createdElement = UiPool::vector3DPool.get().setup(TextFormat("Vector3D %s", property.name.c_str()), this, std::move(propWithComponent));
                const auto element = dynamic_cast<UiVector3D *>(createdElement);
                element->onChanged.subscribe([inspectorStruct, property](const Vector3 &value)
                {
                    property.set(inspectorStruct, value);
                });
            }
            else if (property.type == PropertyType::VECTOR4)
            {
                isSingleField = false;
                createdElement = UiPool::vector4DPool.get().setup(TextFormat("Vector4D %s", property.name.c_str()), this, std::move(propWithComponent));
                const auto element = dynamic_cast<UiVector4D *>(createdElement);
                element->onChanged.subscribe([inspectorStruct, property](const Vector4 &value)
                {
                    property.set(inspectorStruct, value);
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
            else if (property.type == PropertyType::VECTOR_L)
            {
                auto accessorAny = property.get(inspectorStruct);
                const auto *sharedPtr = std::any_cast<std::shared_ptr<VectorAccessor> >(&accessorAny);

                if (!sharedPtr || !*sharedPtr)
                {
                    TraceLog(LOG_ERROR, "Vector accessor is null for %s", property.name.c_str());
                    continue;
                }

                auto &acc = **sharedPtr;
                auto elemType = acc.elementType();

                //TODO Create element for generic vector
            }

            if (!createdElement) continue;

            const auto propNameWidth = propName->getSize().x + propName->getPosition().x;
            createdElement->setAnchor(UI_LEFT_TOP);
            createdElement->setSize({getSize().x, heightSize});
            const auto sizeInPercent = isSingleField ? .3f : 1;
            createdElement->setSizePercentPermanent({sizeInPercent, -1});
            createdElement->setPosition({propNameWidth, verOffset});
        }
    }

    void UiInspector::draw(const float deltaTime)
    {
        if (_isPermanent)
        {
            GuiPanel(_bounds, _componentName.c_str());
        }
        else
        {
            if (GuiWindowBox(_bounds, _componentName.c_str()))
            {
                onDelete.invoke(typeid(_inspectorStruct));
            }
        }

        int i = 1;
        for (const auto &child: getAllChilds())
        {
            if (i == getChildCount() - 1)
            {
                break;
            }

            const auto height = child->getBounds().y + child->getBounds().height + 1.5f;
            DrawLine(child->getBounds().x + 5, height, getBounds().x + getBounds().width - 5, height, LIGHTGRAY);
            i++;
        }
    }

    void UiInspector::update(const float deltaTime)
    {
    }

    bool UiInspector::tryDeleteSelf()
    {
        UiPool::componentPool.release(*this);
        return true;
    }

    void UiInspector::cleanUp()
    {
        destroyAllChilds();
    }
}
