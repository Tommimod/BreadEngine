#include "propertyInspectorWindow.h"
#include "editor.h"
#include "raygui.h"
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
                    _subscriptions.emplace(element, element->onDelete.subscribe([this](const std::type_index type) { this->onUiComponentDeleted(type); }));
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
        auto verticalOffset = _content->getPosition().y + 5;
        constexpr int horizontalOffset = 5;

        _activeCheckBox = &UiPool::checkBoxPool.get().setup(id + "_activeCheckBox", this, "Active", false);
        _activeCheckBox->setAnchor(UI_LEFT_TOP);
        _activeCheckBox->setPosition({horizontalOffset, verticalOffset + 5});
        _activeCheckBox->setSize({10, 10});
        _subscriptions.emplace(_activeCheckBox, _activeCheckBox->onValueChanged.subscribe([this](const bool checked) { this->onNodeActiveChanged(checked); }));

        _nameTextBox = &UiPool::textBoxPool.get().setup(id + "_nameTextBox", this, "Name");
        _nameTextBox->setAnchor(UI_LEFT_TOP);
        _nameTextBox->setPosition({_activeCheckBox->getSize().x + 50, verticalOffset});
        _nameTextBox->setSize({0, 20});
        _nameTextBox->setSizePercentPermanent({.75f, -1});
        _subscriptions.emplace(_nameTextBox, _nameTextBox->onValueChanged.subscribe([this](const char *text) { this->onNodeNameChanged(text); }));

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

    void PropertyInspectorWindow::setupUiComponent(UiInspector *uiComponentElement) const
    {
        uiComponentElement->setAnchor(UI_LEFT_TOP);
        uiComponentElement->setSize({0, 50});
        uiComponentElement->setSizePercentPermanent({.955f, -1});
        auto yPosition = 10.0f;
        if (_activeCheckBox->isActive)
        {
            yPosition += _nameTextBox->getSize().y;
        }

        uiComponentElement->setPosition(Vector2{5, yPosition});
        uiComponentElement->onUpdated += [this](UiInspector *inspector)
        {
            _content->calculateRectForScroll(inspector);
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
} // BreadEditor
