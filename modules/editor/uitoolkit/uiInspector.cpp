#include "uiInspector.h"
#include <thread>

#include "editor.h"
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
        onUpdated.unsubscribeAll();
        _uiListData.clear();
        _isStatic = false;
        _isPermanent = false;
        _componentName.clear();
        _nextInspectorStruct = nullptr;
        _hasNextInspectorStruct = false;

        UiElement::dispose();
    }

    void UiInspector::track(InspectorStruct *inspectorStruct)
    {
        _nextInspectorStruct = inspectorStruct;
        _hasNextInspectorStruct = true;
    }

    void UiInspector::initializeProperties(InspectorStruct *inspectorStruct, std::vector<Property> &properties, int &depth, const float horizonDepth, const int continueFrom, const int vectorIndex)
    {
        for (int i = 0; i < static_cast<int>(properties.size()); i++)
        {
            createSingleElement(i + continueFrom, inspectorStruct, properties[i], nullptr, vectorIndex, depth, horizonDepth);
        }
    }

    void UiInspector::createSingleElement(int order, InspectorStruct *inspectorStruct, Property &property, VectorAccessor *vectorAccessor, int vectorIndex, int &depth, const float horizonDepth)
    {
        constexpr std::string emptyLabel;
        constexpr int uiPropNameLabelWidth = 70;
        auto isSimpleProp = vectorAccessor == nullptr;
        auto withVectorIndex = vectorIndex >= 0;
        float horOffset = 5.0f * horizonDepth;
        constexpr float heightSize = 17;
        const auto verOffset = 5 * static_cast<float>(order + depth) + 30 + heightSize * static_cast<float>(order + depth);
        setSize({_localSize.x, verOffset + heightSize + horOffset});
        computeBounds();

        auto propType = isSimpleProp ? property.type : vectorAccessor->elementType();
        auto propName = propType == PropertyType::INSPECTOR_STRUCT || property.type == PropertyType::VECTOR_L ? property.name + ":" : property.name;
        if (propType == PropertyType::VECTOR_L)
        {
            auto accessorAny = property.get(inspectorStruct);
            propName += "[" + std::to_string(std::any_cast<std::shared_ptr<VectorAccessor> >(&accessorAny)->get()->size()) + "]";
        }

        if (withVectorIndex)
        {
            propName = TextFormat("[%i]%s", vectorIndex, propName.c_str());
        }
        UiLabel *uiPropNameLabel = nullptr;
        if (isSimpleProp)
        {
            uiPropNameLabel = &UiPool::labelPool.get().setup(TextFormat("PropName %s%i", property.name.c_str(), depth), this, propName);
            uiPropNameLabel->setAnchor(UI_LEFT_TOP);
            auto multiplier = 1.0f;
            if (propType == PropertyType::VECTOR_L)
            {
                multiplier = 3;
            }

            uiPropNameLabel->setSize({uiPropNameLabelWidth * multiplier, heightSize});
            uiPropNameLabel->setPosition({horOffset, verOffset});
            uiPropNameLabel->setTextAlignment(TEXT_ALIGN_LEFT);
        }

        UiElement *createdElement = nullptr;
        auto isSingleField = true;
        if (propType == PropertyType::INSPECTOR_STRUCT)
        {
            depth = order + 1;
            auto *structPtr = std::any_cast<InspectorStruct *>(property.get(inspectorStruct));
            initializeProperties(structPtr, structPtr->getInspectedProperties(), depth, horizonDepth + 1);
            depth += static_cast<int>(structPtr->getInspectedProperties().size()) - 2;
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

            auto key = TextFormat("%s%i", property.name.c_str(), order);
            if (!_uiListData.contains(key))
            {
                _uiListData[key] = UiListData(false, sharedPtr);
            }

            const auto isExpanded = _uiListData[key].isExpanded;
            auto expandButton = &UiPool::labelButtonPool.get().setup(TextFormat("Expand %s%i", property.name.c_str(), depth), this,
                                                                     isExpanded ? GuiIconText(ICON_ARROW_DOWN, nullptr) : GuiIconText(ICON_ARROW_RIGHT, nullptr));
            expandButton->setAnchor(UI_LEFT_TOP);
            expandButton->setSize({15, 15});
            expandButton->setPosition({0, verOffset});
            expandButton->onClick.subscribe([this, &property, order](UiLabelButton *button)
            {
                const auto buttonKey = TextFormat("%s%i", property.name.c_str(), order);
                _uiListData[buttonKey].isExpanded = !_uiListData[buttonKey].isExpanded;
                const auto expanded = _uiListData[buttonKey].isExpanded;
                button->setText(expanded ? GuiIconText(ICON_ARROW_DOWN, nullptr) : GuiIconText(ICON_ARROW_RIGHT, nullptr));
                track(_inspectorStruct);
            });

            if (!isExpanded)
            {
                return;
            }

            VectorAccessor &accessor = *_uiListData[key].accessor;
            auto offset = 0;
            auto originalOrder = order;
            depth++;
            auto propNameWidth = isSimpleProp ? static_cast<float>(GuiGetTextWidth(propName.c_str())) + horOffset + 15 : uiPropNameLabelWidth;
            auto addButton = &UiPool::buttonPool.get().setup(TextFormat("AddL %s%i", property.name.c_str(), depth), this, "+");
            addButton->setAnchor(UI_LEFT_TOP);
            addButton->setSize({15, 15});
            addButton->setPosition({propNameWidth, verOffset});
            addButton->onClick.subscribe([this, &property, order](UiButton *)
            {
                const auto buttonKey = TextFormat("%s%i", property.name.c_str(), order);
                _uiListData[buttonKey].accessor->add();
                track(_inspectorStruct);
            });

            auto removeButton = &UiPool::buttonPool.get().setup(TextFormat("RemoveL %s%i", property.name.c_str(), depth), this, "-");
            removeButton->setAnchor(UI_LEFT_TOP);
            removeButton->setSize({15, 15});
            removeButton->setPosition({propNameWidth + 20, verOffset});
            removeButton->onClick.subscribe([this, &property, order](UiButton *)
            {
                const auto buttonKey = TextFormat("%s%i", property.name.c_str(), order);
                const auto acc = _uiListData[buttonKey].accessor;
                acc->remove(acc->size() - 1);
                track(_inspectorStruct);
            });
            for (auto index = 0; index < static_cast<int>(accessor.size()); index++)
            {
                if (accessor.elementType() == PropertyType::INSPECTOR_STRUCT)
                {
                    auto structPtr = std::any_cast<InspectorStruct *>(accessor.get(index));
                    initializeProperties(structPtr, structPtr->getInspectedProperties(), depth, horizonDepth + 1, order, index);
                    offset += static_cast<int>(structPtr->getInspectedProperties().size());
                    order = originalOrder + offset;
                }
                else
                {
                    createSingleElement(order + index, inspectorStruct, property, &accessor, index, depth, horizonDepth + 1);
                }
            }

            depth += offset - 1;
        }

        if (!createdElement) return;

        auto propNameWidth = isSimpleProp ? uiPropNameLabelWidth + horOffset : uiPropNameLabelWidth;
        if (propType == PropertyType::VECTOR_L)
        {
            propNameWidth += 15;
        }

        createdElement->setAnchor(UI_LEFT_TOP);
        createdElement->setSize({0, heightSize});
        const auto sizeInPercent = isSingleField ? .6f : 1;
        createdElement->setSizePercentPermanent({sizeInPercent, -1});
        if (isSimpleProp)
        {
            createdElement->setPosition({propNameWidth, verOffset});
        }
        else
        {
            createdElement->setPosition({horOffset, verOffset});
        }

        _fields.emplace_back(createdElement);
    }

    void UiInspector::draw(const float deltaTime)
    {
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::Medium));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::Medium));
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
        for (const auto &child: _fields)
        {
            if (i == getChildCount() - 1)
            {
                break;
            }

            const auto height = child->getBounds().y + child->getBounds().height + 1.5f;
            DrawLine(getBounds().x + 5, height, getBounds().x + getBounds().width - 5, height, LIGHTGRAY);
            i++;
        }
    }

    void UiInspector::update(const float deltaTime)
    {
        if (_nextInspectorStruct == nullptr || !_hasNextInspectorStruct)
        {
            return;
        }

        if (_inspectorStruct != _nextInspectorStruct)
        {
            _uiListData.clear();
        }

        if (getChildCount() > 0)
        {
            cleanUp();
        }
        else
        {
            _hasNextInspectorStruct = false;
            constexpr std::string transformName = "Transform";
            _inspectorStruct = _nextInspectorStruct;
            _componentName = _nextInspectorStruct->getTypeName();
            _isPermanent = _isStatic || _componentName == transformName;
            int depth = 0;
            TraceLog(LOG_DEBUG, "______REDRAW_____");
            initializeProperties(_nextInspectorStruct, _nextInspectorStruct->getInspectedProperties(), depth, 1);
            onUpdated.invoke(this);
        }
    }

    bool UiInspector::tryDeleteSelf()
    {
        UiPool::componentPool.release(*this);
        return true;
    }

    void UiInspector::cleanUp()
    {
        destroyAllChilds();
        _fields.clear();
    }
}
