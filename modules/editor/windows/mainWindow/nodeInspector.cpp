#include "nodeInspector.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string NodeInspector::Id = "mainWindowNodeInspector";

    NodeInspector::NodeInspector(const std::string &id)
    {
        setup(id);
        isVerticalResized = true;
        initialize();
    }

    NodeInspector::NodeInspector(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        isVerticalResized = true;
        initialize();
    }

    NodeInspector::~NodeInspector()
    {
        _activeCheckBox->onStateChanged.unsubscribe(_subscriptions[_activeCheckBox]);
        _nameTextBox->onTextChanged.unsubscribe(_subscriptions[_nameTextBox]);

        _subscriptions.clear();
    }

    void NodeInspector::draw(float deltaTime)
    {
        GuiPanel(_bounds, nullptr);
        const auto isDisabled = _engineNode == nullptr;
        for (auto childElement: _childs)
        {
            childElement->setState(isDisabled ? STATE_DISABLED : STATE_NORMAL);
        }

        UiElement::draw(deltaTime);
    }

    void NodeInspector::update(float deltaTime)
    {
        updateResizable(*this);
        UiElement::update(deltaTime);
    }

    void NodeInspector::lookupNode(Node *node)
    {
        _engineNode = node;
        if (_engineNode != nullptr)
        {
            _activeCheckBox->setChecked(_engineNode->getIsActive());
            _nameTextBox->setText(_engineNode->getName());
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

    void NodeInspector::initialize()
    {
        const int verticalOffset = 5;
        const int horizontalOffset = 5;

        _activeCheckBox = &UiPool::checkBoxPool.get().setup("nodeInspectorActiveCheckBox", this, false, "Active");
        _activeCheckBox->setAnchor(UI_LEFT_TOP);
        _activeCheckBox->setPosition({horizontalOffset, verticalOffset + 5});
        _activeCheckBox->setSize({10, 10});
        _subscriptions.emplace(_activeCheckBox, _activeCheckBox->onStateChanged.subscribe([this](bool checked) { this->onNodeActiveChanged(checked); }));

        _nameTextBox = &UiPool::textBoxPool.get().setup("nodeInspectorNameTextBox", this, "Name");
        _nameTextBox->setAnchor(UI_LEFT_TOP);
        _nameTextBox->setPosition({_activeCheckBox->getSize().x + 50, verticalOffset});
        _nameTextBox->setSize({100, 20});
        _subscriptions.emplace(_nameTextBox, _nameTextBox->onTextChanged.subscribe([this](char *text) { this->onNodeNameChanged(text); }));

        resetElementsState();
    }

    void NodeInspector::resetElementsState()
    {
        const auto emptyString = "";
        _activeCheckBox->setChecked(false);
        _nameTextBox->setText(emptyString);
    }

    void NodeInspector::onNodeActiveChanged(bool isActive)
    {
        _engineNode->setIsActive(isActive);
    }

    void NodeInspector::onNodeNameChanged(const char *name)
    {
        _engineNode->setName(name);
    }
} // BreadEditor
