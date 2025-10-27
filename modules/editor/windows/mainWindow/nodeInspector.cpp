#include "nodeInspector.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
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
        activeCheckBox->onStateChanged.unsubscribe(subscriptions[activeCheckBox]);
        nameTextBox->onTextChanged.unsubscribe(subscriptions[nameTextBox]);

        subscriptions.clear();
    }

    void NodeInspector::draw(float deltaTime)
    {
        GuiPanel(bounds, nullptr);
        const auto isDisabled = engineNode == nullptr;
        for (auto childElement: childs)
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
        engineNode = node;
        if (engineNode != nullptr)
        {
            activeCheckBox->setChecked(engineNode->getIsActive());
            nameTextBox->setText(engineNode->getName());
        }
        else
        {
            resetElementsState();
        }
    }

    void NodeInspector::clear()
    {
        engineNode = nullptr;
    }

    void NodeInspector::deleteSelf()
    {
    }

    void NodeInspector::initialize()
    {
        const int verticalOffset = 5;
        const int horizontalOffset = 5;

        activeCheckBox = &UiPool::checkBoxPool.get().setup("nodeInspectorActiveCheckBox", this, false, "Active");
        activeCheckBox->setAnchor(UI_LEFT_TOP);
        activeCheckBox->setPosition({horizontalOffset, verticalOffset + 5});
        activeCheckBox->setSize({10, 10});
        subscriptions.emplace(activeCheckBox, activeCheckBox->onStateChanged.subscribe([this](bool checked) { this->onNodeActiveChanged(checked); }));

        nameTextBox = &UiPool::textBoxPool.get().setup("nodeInspectorNameTextBox", this, "Name");
        nameTextBox->setAnchor(UI_LEFT_TOP);
        nameTextBox->setPosition({activeCheckBox->getSize().x + 50, verticalOffset});
        nameTextBox->setSize({100, 20});
        subscriptions.emplace(nameTextBox, nameTextBox->onTextChanged.subscribe([this](char *text) { this->onNodeNameChanged(text); }));

        resetElementsState();
    }

    void NodeInspector::resetElementsState()
    {
        const auto emptyString = "";
        activeCheckBox->setChecked(false);
        nameTextBox->setText(emptyString);
    }

    void NodeInspector::onNodeActiveChanged(bool isActive)
    {
        engineNode->setIsActive(isActive);
    }

    void NodeInspector::onNodeNameChanged(const char *name)
    {
        engineNode->setName(name);
    }
} // BreadEditor
