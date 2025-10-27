#include "nodeUiElement.h"

#include "engine.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeUiElement::NodeUiElement() = default;

    NodeUiElement::~NodeUiElement() = default;

    NodeUiElement &NodeUiElement::setup(const std::string &id, UiElement *parentElement, Node *node)
    {
        this->engineNode = node;
        return dynamic_cast<NodeUiElement &>(UiElement::setup(id, parentElement));
    }

    void NodeUiElement::draw(const float deltaTime)
    {
        if (parentNode != nullptr)
        {
            auto parentState = parentNode->state;
            state = std::max(localState, parentState);
        }

        GuiSetState(state);
        if (GuiButton(bounds, nullptr) || (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && Engine::IsCollisionPointRec(GetMousePosition(), bounds)))
        {
            onSelected.invoke(this);
        }

        const auto buttonText = GuiIconText(ICON_CPU, engineNode->getName().c_str());
        GuiLabel(bounds, buttonText);
        UiElement::draw(deltaTime);
    }

    void NodeUiElement::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    Node *NodeUiElement::getNode() const
    {
        return engineNode;
    }

    void NodeUiElement::setParentNode(NodeUiElement *nextParentNode)
    {
        this->parentNode = nextParentNode;
    }

    void NodeUiElement::setEngineNode(Node *nextEngineNode)
    {
        this->engineNode = nextEngineNode;
    }

    void NodeUiElement::setState(GuiState nextState)
    {
        UiElement::setState(nextState);
        localState = nextState;
    }

    void NodeUiElement::deleteSelf()
    {
        UiPool::nodeUiElementPool.release(*this);
    }
} // BreadEditor
