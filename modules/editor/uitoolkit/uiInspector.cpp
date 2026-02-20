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
        _uiListData.clear();
        _isStatic = false;
        _isPermanent = false;
        _componentName.clear();

        UiElement::dispose();
    }

    void UiInspector::track(InspectorStruct *inspectorStruct)
    {
        if (_inspectorStruct != inspectorStruct)
        {
            _uiListData.clear();
        }

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
            int depth = 0;
            initializeProperties(inspectorStruct, inspectorStruct->getInspectedProperties(), depth, 1);
        });

        workerThread.detach();
    }

    void UiInspector::initializeProperties(InspectorStruct *inspectorStruct, std::vector<Property> &properties, int &depth, const float horizonDepth)
    {
        for (int i = 0; i < static_cast<int>(properties.size()); i++)
        {
            createSingleElement(i, inspectorStruct, properties[i], nullptr, 0, depth, horizonDepth);
        }
    }

    void UiInspector::createSingleElement(int order, InspectorStruct *inspectorStruct, Property &property, VectorAccessor *vectorAccessor, int vectorIndex, int &depth, const float horizonDepth)
    {
        constexpr std::string emptyLabel;
        constexpr int uiPropNameLabelWidth = 70;
        auto isSimpleProp = vectorAccessor == nullptr;
        float horOffset = 5.0f * horizonDepth;
        constexpr float heightSize = 17;
        const auto verOffset = 5 * static_cast<float>(order + depth) + 30 + heightSize * static_cast<float>(order + depth);
        setSize({_localSize.x, verOffset + heightSize + horOffset});

        auto propType = property.type;
        auto propName = propType == PropertyType::INSPECTOR_STRUCT || property.type == PropertyType::VECTOR_L ? property.name + ":" : property.name;
        if (isSimpleProp)
        {
            const auto uiPropNameLabel = UiPool::labelPool.get().setup(TextFormat("PropName %s%i", property.name.c_str(), depth), this, propName);
            uiPropNameLabel->setAnchor(UI_LEFT_TOP);
            uiPropNameLabel->setSize({uiPropNameLabelWidth, heightSize});
            uiPropNameLabel->setPosition({horOffset, verOffset});
            uiPropNameLabel->computeBounds();
        }

        UiElement *createdElement = nullptr;
        auto isSingleField = true;
        if (propType == PropertyType::INSPECTOR_STRUCT)
        {
            depth = order + 1;
            auto *structPtr = std::any_cast<InspectorStruct *>(property.get(inspectorStruct));
            initializeProperties(structPtr, structPtr->getInspectedProperties(), depth, horizonDepth + 1);
        }
        else if (propType == PropertyType::INT)
        {
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<int>(property.get(inspectorStruct));
                };

                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("IntBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<int>(vectorAccessor->get(vectorIndex));
                };

                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("IntBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiNumberBox *>(createdElement);
            element->onValueChanged.subscribe([inspectorStruct, property](const int &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::FLOAT)
        {
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<float>(property.get(inspectorStruct));
                };

                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("FloatBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<float>(vectorAccessor->get(vectorIndex));
                };

                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("FloatBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiNumberBox *>(createdElement);
            element->onValueChanged.subscribe([inspectorStruct, property](const float &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::LONG)
        {
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<long>(property.get(inspectorStruct));
                };

                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("LongBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<long>(vectorAccessor->get(vectorIndex));
                };

                createdElement = &UiPool::numberBoxPool.get().setup(TextFormat("LongBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiNumberBox *>(createdElement);
            element->onValueChanged.subscribe([inspectorStruct, property](const long &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::BOOL)
        {
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<bool>(property.get(inspectorStruct));
                };

                createdElement = &UiPool::checkBoxPool.get().setup(TextFormat("CheckBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<bool>(vectorAccessor->get(vectorIndex));
                };

                createdElement = &UiPool::checkBoxPool.get().setup(TextFormat("CheckBox %s%i", property.name.c_str(), depth), this, emptyLabel, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiCheckBox *>(createdElement);
            element->onValueChanged.subscribe([inspectorStruct, property](const bool &value)
            {
                property.set(inspectorStruct, value);
            });
            element->setSizeMax({heightSize, heightSize});
        }
        else if (propType == PropertyType::STRING)
        {
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<std::string>(property.get(inspectorStruct));
                };

                createdElement = &UiPool::textBoxPool.get().setup(TextFormat("TextBox %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<std::string>(vectorAccessor->get(vectorIndex));
                };

                createdElement = &UiPool::textBoxPool.get().setup(TextFormat("TextBox %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiTextBox *>(createdElement);
            element->onValueChanged.subscribe([inspectorStruct, property](const std::string &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::VECTOR2)
        {
            isSingleField = false;

            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<Vector2>(property.get(inspectorStruct));
                };

                createdElement = UiPool::vector2DPool.get().setup(TextFormat("Vector2D %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<Vector2>(vectorAccessor->get(vectorIndex));
                };

                createdElement = UiPool::vector2DPool.get().setup(TextFormat("Vector2D %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiVector2D *>(createdElement);
            element->onChanged.subscribe([inspectorStruct, property](const Vector2 &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::VECTOR3)
        {
            isSingleField = false;
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<Vector3>(property.get(inspectorStruct));
                };

                createdElement = UiPool::vector3DPool.get().setup(TextFormat("Vector3D %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<Vector3>(vectorAccessor->get(vectorIndex));
                };

                createdElement = UiPool::vector3DPool.get().setup(TextFormat("Vector3D %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiVector3D *>(createdElement);
            element->onChanged.subscribe([inspectorStruct, property](const Vector3 &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::VECTOR4)
        {
            isSingleField = false;
            if (isSimpleProp)
            {
                auto getFunc = [inspectorStruct, property]
                {
                    return std::any_cast<Vector4>(property.get(inspectorStruct));
                };

                createdElement = UiPool::vector4DPool.get().setup(TextFormat("Vector4D %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            else
            {
                auto getFunc = [vectorAccessor, vectorIndex]
                {
                    return std::any_cast<Vector4>(vectorAccessor->get(vectorIndex));
                };

                createdElement = UiPool::vector4DPool.get().setup(TextFormat("Vector4D %s%i", property.name.c_str(), depth), this, std::move(getFunc));
            }
            const auto element = dynamic_cast<UiVector4D *>(createdElement);
            element->onChanged.subscribe([inspectorStruct, property](const Vector4 &value)
            {
                property.set(inspectorStruct, value);
            });
        }
        else if (propType == PropertyType::COLOR)
        {
            //TODO
        }
        else if (propType == PropertyType::ENUM)
        {
            //TODO
        }
        else if (propType == PropertyType::VECTOR_L)
        {
            auto accessorAny = property.get(inspectorStruct);
            const auto *sharedPtr = std::any_cast<std::shared_ptr<VectorAccessor> >(&accessorAny);

            if (!sharedPtr || !*sharedPtr)
            {
                TraceLog(LOG_ERROR, "Vector accessor is null for %s", property.name.c_str());
                return;
            }

            auto expandButton = &UiPool::labelButtonPool.get().setup(TextFormat("Expand %s%i", property.name.c_str(), depth), this, GuiIconText(ICON_ARROW_RIGHT, nullptr));
            expandButton->setAnchor(UI_LEFT_TOP);
            expandButton->setSize({15, 15});
            expandButton->setPosition({0, verOffset});

            if (!_uiListData.contains(&property))
            {
                _uiListData[&property] = UiListData(false, sharedPtr);
            }

            expandButton->onClick.subscribe([this, &property](UiLabelButton *button)
            {
                _uiListData[&property].isExpanded = !_uiListData[&property].isExpanded;
                const auto isExpanded = _uiListData[&property].isExpanded;
                button->setText(isExpanded ? GuiIconText(ICON_ARROW_DOWN, nullptr) : GuiIconText(ICON_ARROW_RIGHT, nullptr));
                track(_inspectorStruct);
            });
            if (const auto isExpanded = _uiListData[&property].isExpanded; !isExpanded)
            {
                return;
            }

            VectorAccessor &accessor = *_uiListData[&property].accessor;
            for (auto index = 0; index < static_cast<int>(accessor.size()); index++)
            {
                depth++;
                if (accessor.elementType() == PropertyType::INSPECTOR_STRUCT)
                {
                    auto structPtr = std::any_cast<InspectorStruct>(accessor.get(index));
                    initializeProperties(&structPtr, structPtr.getInspectedProperties(), depth, horizonDepth + 1);
                    continue;
                }

                createSingleElement(order, inspectorStruct, property, &accessor, index, depth, horizonDepth + 1);
            }
        }

        if (!createdElement) return;

        const auto propNameWidth = isSimpleProp ? uiPropNameLabelWidth + horOffset : uiPropNameLabelWidth;
        createdElement->setAnchor(UI_LEFT_TOP);
        createdElement->setSize({getSize().x, heightSize});
        const auto sizeInPercent = isSingleField ? .3f : 1;
        createdElement->setSizePercentPermanent({sizeInPercent, -1});
        createdElement->setPosition({propNameWidth, verOffset});
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
