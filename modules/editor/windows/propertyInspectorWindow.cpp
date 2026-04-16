#include "propertyInspectorWindow.h"
#include "editor.h"
#include "commands/commandsHandler.h"
#include "commands/inspectorCommands/addComponentCommand.h"
#include "commands/inspectorCommands/changeNameNodeCommand.h"
#include "commands/inspectorCommands/setActiveNodeCommand.h"
#include "component/camera.h"
#include "editorElements/customDropdownUiElement.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string PropertyInspectorWindow::Id = "Inspector";

    PropertyInspectorWindow::PropertyInspectorWindow(const std::string_view &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
        rebuild();
    }

    PropertyInspectorWindow::PropertyInspectorWindow(const std::string_view &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
        rebuild();
    }

    PropertyInspectorWindow::~PropertyInspectorWindow()
    {
        _activeCheckBox->onValueChanged.unsubscribe(_subscriptions[_activeCheckBox]);
        _nameTextBox->onValueChanged.unsubscribe(_subscriptions[_nameTextBox]);
        for (const auto uiComponent: _uiComponentElements)
        {
            uiComponent->onDelete.unsubscribe(_subscriptions[uiComponent]);
        }

        _uiComponentElements.clear();
        _subscriptions.clear();
    }

    void PropertyInspectorWindow::dispose()
    {
        UiWindow::dispose();
    }

    void PropertyInspectorWindow::lookupNode(Node *node)
    {
        if (_engineNode == node)
        {
            return;
        }

        resetElementsState(true);
        clear();
        _engineNode = node;
        if (_engineNode != nullptr)
        {
            _activeCheckBox->setChecked(_engineNode->getIsActive());
            _nameTextBox->setText(_engineNode->getName());
            _trackedComponents = ComponentsProvider::getAllComponents(_engineNode->getId());
            if (const int needCreateCount = static_cast<int>(_trackedComponents.size() - _uiComponentElements.size()); needCreateCount > 0)
            {
                for (int i = static_cast<int>(_trackedComponents.size()) - needCreateCount; i < static_cast<int>(_trackedComponents.size()); i++)
                {
                    auto element = &UiPool::componentPool.get().setup("UiComponent_" + std::to_string(_uiComponentElements.size()), _content, false);
                    _uiComponentElements.push_back(element);
                    setupUiComponent(element);
                    _subscriptions.emplace(element, element->onDelete.subscribe([this](const std::type_index type)
                    {
                        onUiComponentDeleted(type);
                    }));
                }
            }
            else if (needCreateCount < 0)
            {
                for (int i = static_cast<int>(_uiComponentElements.size()); i != static_cast<int>(_trackedComponents.size()); i--)
                {
                    const auto element = _uiComponentElements.back();
                    _uiComponentElements.pop_back();
                    element->onDelete.unsubscribe(_subscriptions[element]);
                    _content->destroyChild(element);
                }
            }

            for (int i = 0; i < static_cast<int>(_trackedComponents.size()); i++)
            {
                _uiComponentElements[i]->track(_trackedComponents[i]);
            }

            if (static_cast<int>(_trackedComponents.size()) == 0)
            {
                adjustAddComponentButtonPosition();
            }
        }
        else
        {
            resetElementsState(true);
        }
    }

    void PropertyInspectorWindow::lookupStruct(InspectorStruct *inspectorStruct)
    {
        if (_inspectorStruct == inspectorStruct)
        {
            return;
        }

        clear();
        resetElementsState(false);
        _inspectorStruct = inspectorStruct;
        for (int i = static_cast<int>(_uiComponentElements.size()); i != 0; i--)
        {
            const auto element = _uiComponentElements.back();
            _uiComponentElements.pop_back();
            element->onDelete.unsubscribe(_subscriptions[element]);
            _content->destroyChild(element);
        }

        const auto element = &UiPool::componentPool.get().setup("UiStruct_0", _content, true);
        _uiComponentElements.push_back(element);
        setupUiComponent(element);
        element->track(inspectorStruct);
    }

    void PropertyInspectorWindow::clear()
    {
        _engineNode = nullptr;
        _inspectorStruct = nullptr;
    }

    void PropertyInspectorWindow::awake()
    {
        UiWindow::awake();
        const auto verticalOffset = _content->getPosition().y + 5;
        constexpr int horizontalOffset = 5;

        _activeCheckBox = &UiPool::checkBoxPool.get().setup(id + "_activeCheckBox", this, "Active", false);
        _activeCheckBox->setAnchor(UI_LEFT_TOP);
        _activeCheckBox->setPosition({horizontalOffset, verticalOffset + 5});
        _activeCheckBox->setSize({10, 10});
        _activeCheckBox->onValueChanged.subscribe([this](const bool checked)
        {
            CommandsHandler::execute(std::make_unique<SetActiveNodeCommand>(_engineNode, _engineNode->getIsActive()));
            this->onNodeActiveChanged(checked);
        });

        _nameTextBox = &UiPool::textBoxPool.get().setup(id + "_nameTextBox", this, "Name");
        _nameTextBox->setAnchor(UI_LEFT_TOP);
        _nameTextBox->setPosition({_activeCheckBox->getSize().x + 50, verticalOffset});
        _nameTextBox->setSize({0, 20});
        _nameTextBox->setSizePercentPermanent({.75f, -1});
        _nameTextBox->onValueChanged.subscribe([this](const char *text)
        {
            CommandsHandler::execute(std::make_unique<ChangeNameNodeCommand>(_engineNode, text, _engineNode->getName()));
            this->onNodeNameChanged(text);
        });

        _componentsDropdown = std::make_unique<CustomDropdownUiElement>();
        _componentsDropdown->setup(id + "_componentsDropdown", this, {});
        _componentsDropdown->setPivot({.5f, 0});
        _componentsDropdown->setAnchor(UI_CENTER_TOP);
        _componentsDropdown->setSize({250, 350});
        _componentsDropdown->setPosition({0, 15});
        _componentsDropdown->isActive = false;

        _addComponentButton = &UiPool::buttonPool.get().setup(id + "_addComponentButton", this, "Add Component");
        _addComponentButton->setAnchor(UI_LEFT_TOP);
        _addComponentButton->setPivot({.5f, -1});
        _addComponentButton->setSizePercentPermanent({.55f, -1});
        _addComponentButton->setSize({-1, 25});
        _addComponentButton->onClick.subscribe([this](UiButton *)
        {
            if (_engineNode == nullptr) return;

            auto allComponentsInEngineList = ComponentRegistry::getAllComponents();
            auto nodeComponents = ComponentsProvider::getAllComponentTypes(_engineNode->getId());
            allComponentsInEngineList.erase(ranges::remove_if(allComponentsInEngineList,
                                                              [nodeComponents](const std::type_index &type)
                                                              {
                                                                  return ranges::find(nodeComponents, type) != nodeComponents.end();
                                                              }).begin());
            auto options = std::vector<std::string>();
            for (const auto &val: allComponentsInEngineList)
            {
                if (val == typeid(BreadEngine::Transform)) continue;
                options.emplace_back(ComponentRegistry::getComponentNameByType(val));
            }

            _componentsDropdown->isActive = true;
            _componentsDropdown->setOptions(options);
            _componentsDropdown->onOptionSelected.subscribe([this](const std::string &value)
            {
                _componentsDropdown->close();
                if (value.empty()) return;

                const auto id = _engineNode->getId();
                ComponentsProvider::addDynamic(id, value);
                CommandsHandler::execute(std::make_unique<AddComponentCommand>(_engineNode, value));
                const auto node = NodeProvider::getNode(id);
                _engineNode = nullptr;
                lookupNode(node);
            });
        });

        setChildLast(_componentsDropdown.get());
        resetElementsState(false);
    }

    void PropertyInspectorWindow::subscribe()
    {
        Editor::getInstance().getEditorModel().onClearSelection.subscribe([this]
        {
            clear();
            resetElementsState(false);
            for (int i = static_cast<int>(_uiComponentElements.size()); i != 0; i--)
            {
                const auto element = _uiComponentElements.back();
                _uiComponentElements.pop_back();
                element->onDelete.unsubscribe(_subscriptions[element]);
                _content->destroyChild(element);
            }
        });

        UiWindow::subscribe();
    }

    void PropertyInspectorWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }

    void PropertyInspectorWindow::resetElementsState(const bool withGlobalSettings)
    {
        const auto emptyString = "";
        _activeCheckBox->isActive = withGlobalSettings;
        _nameTextBox->isActive = withGlobalSettings;
        _addComponentButton->isActive = withGlobalSettings;
        _activeCheckBox->setChecked(false);
        _nameTextBox->setText(emptyString);
        _trackedComponents = {};
    }

    void PropertyInspectorWindow::rebuild()
    {
        const auto selectedNode = Editor::getInstance().getEditorModel().getSelectedEngineNode();
        if (!selectedNode) return;
        lookupNode(selectedNode);
    }

    void PropertyInspectorWindow::onNodeActiveChanged(const bool isActive) const
    {
        _engineNode->setIsActive(isActive);
        _activeCheckBox->setChecked(_engineNode->getIsActive());
    }

    void PropertyInspectorWindow::onNodeNameChanged(const char *name) const
    {
        _engineNode->setName(name);
        _nameTextBox->setText(_engineNode->getName());
    }

    void PropertyInspectorWindow::setupUiComponent(UiInspector *uiInspectorElement) const
    {
        uiInspectorElement->setAnchor(UI_LEFT_TOP);
        uiInspectorElement->setSize({0, 50});
        uiInspectorElement->setSizePercentPermanent({.955f, -1});
        auto yPosition = 10.0f;
        if (_activeCheckBox->isActive)
        {
            yPosition += _nameTextBox->getSize().y;
        }

        uiInspectorElement->onUpdated += [this, yPosition](UiInspector *)
        {
            auto prevPos = yPosition;
            const auto allChilds = _content->getAllChilds();
            for (int i = allChilds.size() - 1; i >= 0; --i)
            {
                const auto child = allChilds[i];
                child->setPosition({5, prevPos});
                prevPos += child->getSize().y + 5;
            }
            adjustAddComponentButtonPosition();
        };
    }

    void PropertyInspectorWindow::onUiComponentDeleted(const std::type_index type)
    {
        _engineNode->remove(type);
        lookupNode(_engineNode);
    }

    void PropertyInspectorWindow::initializePanel()
    {
    }

    void PropertyInspectorWindow::cleanupPanel()
    {
    }

    void PropertyInspectorWindow::adjustAddComponentButtonPosition() const
    {
        const auto lastInspector = _content->getAllChilds()[0];
        const auto size = Vector2{lastInspector->getPosition().x + lastInspector->getSize().x * .5f, lastInspector->getPosition().y + lastInspector->getSize().y + 10};
        _addComponentButton->setPosition(size);
        _content->calculateRectForScroll(_addComponentButton);
    }
} // BreadEditor
