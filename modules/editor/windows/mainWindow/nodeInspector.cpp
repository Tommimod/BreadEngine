#include "nodeInspector.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string NodeInspector::Id = "mainWindowNodeInspector";

    NodeInspector::NodeInspector(const std::string &id)
    {
        setup(id);
        initialize();
    }

    NodeInspector::NodeInspector(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        initialize();
    }

    NodeInspector::~NodeInspector()
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

    void NodeInspector::draw(const float deltaTime)
    {
        GuiSetState(_state);
        GuiPanel(_bounds, nullptr);
        const auto isDisabled = _engineNode == nullptr;
        for (const auto childElement: _childs)
        {
            childElement->setState(isDisabled ? STATE_DISABLED : STATE_NORMAL);
        }

        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
        updateResizable(*this);
    }

    void NodeInspector::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void NodeInspector::lookupNode(Node *node)
    {
        _engineNode = node;
        if (_engineNode != nullptr)
        {
            _activeCheckBox->setChecked(_engineNode->getIsActive());
            _nameTextBox->setText(_engineNode->getName());
            _trackedComponents = ComponentsProvider::getAllComponents(_engineNode->getId());
            if (const int needCreateCount = _trackedComponents.size() - _uiComponentElements.size(); needCreateCount > 0)
            {
                for (int i = static_cast<int>(_trackedComponents.size()) - needCreateCount; i < static_cast<int>(_trackedComponents.size()); i++)
                {
                    auto element = &UiPool::componentPool.get().setup("UiComponent_" + std::to_string(_uiComponentElements.size()), this);
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
                    element->dispose();
                    _uiComponentElements.pop_back();
                    element->onDelete.unsubscribe(_subscriptions[element]);
                }
            }

            for (int i = 0; i < static_cast<int>(_trackedComponents.size()); i++)
            {
                _uiComponentElements[i]->trackComponent(_trackedComponents[i]);
            }
        }
        else
        {
            resetElementsState();
        }
    }

    void NodeInspector::clear()
    {
        _engineNode = nullptr;
    }

    bool NodeInspector::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }

    void NodeInspector::initialize()
    {
        constexpr int verticalOffset = 5;
        constexpr int horizontalOffset = 5;

        _activeCheckBox = &UiPool::checkBoxPool.get().setup("nodeInspectorActiveCheckBox", this, "Active", false);
        _activeCheckBox->setAnchor(UI_LEFT_TOP);
        _activeCheckBox->setPosition({horizontalOffset, verticalOffset + 5});
        _activeCheckBox->setSize({10, 10});
        _subscriptions.emplace(_activeCheckBox, _activeCheckBox->onValueChanged.subscribe([this](const bool checked) { this->onNodeActiveChanged(checked); }));

        _nameTextBox = &UiPool::textBoxPool.get().setup("nodeInspectorNameTextBox", this, "Name");
        _nameTextBox->setAnchor(UI_LEFT_TOP);
        _nameTextBox->setPosition({_activeCheckBox->getSize().x + 50, verticalOffset});
        _nameTextBox->setSize({100, 20});
        _subscriptions.emplace(_nameTextBox, _nameTextBox->onValueChanged.subscribe([this](const char *text) { this->onNodeNameChanged(text); }));

        resetElementsState();
    }

    void NodeInspector::resetElementsState()
    {
        const auto emptyString = "";
        _activeCheckBox->setChecked(false);
        _nameTextBox->setText(emptyString);
        _trackedComponents = {};
    }

    void NodeInspector::onNodeActiveChanged(const bool isActive) const
    {
        _engineNode->setIsActive(isActive);
    }

    void NodeInspector::onNodeNameChanged(const char *name) const
    {
        _engineNode->setName(name);
    }

    void NodeInspector::setupUiComponent(UiComponent *uiComponentElement) const
    {
        uiComponentElement->setAnchor(UI_LEFT_TOP);
        uiComponentElement->setSize(Vector2{_bounds.width - 10, 50});
        uiComponentElement->setSizePercentPermanent({.97f, -1});
        const auto yPosition = _nameTextBox->getPosition().y + _nameTextBox->getSize().y + 10;
        uiComponentElement->setPosition(Vector2{5, yPosition});
    }

    void NodeInspector::onUiComponentDeleted(const std::type_index type)
    {
        _engineNode->remove(type);
        lookupNode(_engineNode);
    }
} // BreadEditor
