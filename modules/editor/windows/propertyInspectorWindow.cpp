#include "propertyInspectorWindow.h"
#include "editor.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string PropertyInspectorWindow::Id = "Inspector";

    PropertyInspectorWindow::PropertyInspectorWindow(const std::string &id) : UiWindow(id)
    {
        setup(id);
        initialize();
        subscribe();
        rebuild();
    }

    PropertyInspectorWindow::PropertyInspectorWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        initialize();
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

    void PropertyInspectorWindow::draw(const float deltaTime)
    {
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
    }

    void PropertyInspectorWindow::update(const float deltaTime)
    {
        UiWindow::update(deltaTime);
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
                    auto element = &UiPool::componentPool.get().setup("UiComponent_" + std::to_string(_uiComponentElements.size()), this, false);
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
                    destroyChild(element);
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
            destroyChild(element);
        }

        const auto element = &UiPool::componentPool.get().setup("UiStruct_0", this, true);
        _uiComponentElements.push_back(element);
        setupUiComponent(element);
        element->track(inspectorStruct);
    }

    void PropertyInspectorWindow::clear()
    {
        _engineNode = nullptr;
        _inspectorStruct = nullptr;
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
                destroyChild(element);
            }
        });

        UiWindow::subscribe();
    }

    void PropertyInspectorWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }

    void PropertyInspectorWindow::initialize()
    {
        constexpr int verticalOffset = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 10;
        constexpr int horizontalOffset = 5;

        _activeCheckBox = &UiPool::checkBoxPool.get().setup("nodeInspectorActiveCheckBox", this, "Active", false);
        _activeCheckBox->setAnchor(UI_LEFT_TOP);
        _activeCheckBox->setPosition({horizontalOffset, verticalOffset + 5});
        _activeCheckBox->setSize({10, 10});
        _subscriptions.emplace(_activeCheckBox, _activeCheckBox->onValueChanged.subscribe([this](const bool checked) { this->onNodeActiveChanged(checked); }));

        _nameTextBox = &UiPool::textBoxPool.get().setup("nodeInspectorNameTextBox", this, "Name");
        _nameTextBox->setAnchor(UI_LEFT_TOP);
        _nameTextBox->setPosition({_activeCheckBox->getSize().x + 50, verticalOffset});
        _nameTextBox->setSize({0, 20});
        _nameTextBox->setSizePercentPermanent({.75f, -1});
        _subscriptions.emplace(_nameTextBox, _nameTextBox->onValueChanged.subscribe([this](const char *text) { this->onNodeNameChanged(text); }));

        resetElementsState(true);
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
        const auto selectedElement = Editor::getInstance().getEditorModel().getSelectedNodeUiElement();
        if (!selectedElement) return;
        lookupNode(selectedElement->getNode());
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

    void PropertyInspectorWindow::setupUiComponent(UiInspector *uiComponentElement)
    {
        uiComponentElement->setAnchor(UI_LEFT_TOP);
        uiComponentElement->setSize({0, 50});
        uiComponentElement->setSizePercentPermanent({.955f, -1});
        auto yPosition = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 10.0f;
        if (_activeCheckBox->isActive)
        {
            yPosition = _nameTextBox->getPosition().y + _nameTextBox->getSize().y + 10;
        }

        uiComponentElement->setPosition(Vector2{5, yPosition});
        uiComponentElement->onUpdated += [this](UiInspector *inspector)
        {
            calculateRectForScroll(inspector);
        };
    }

    void PropertyInspectorWindow::onUiComponentDeleted(const std::type_index type)
    {
        _engineNode->remove(type);
        lookupNode(_engineNode);
    }
} // BreadEditor
