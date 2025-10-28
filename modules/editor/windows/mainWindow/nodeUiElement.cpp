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
        state = getState();
        GuiSetState(state);
        if (GuiButton(bounds, nullptr) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Engine::IsCollisionPointRec(GetMousePosition(), bounds)))
        {
            onSelected.invoke(this);
        }

        const auto buttonText = GuiIconText(ICON_CPU, engineNode->getName().c_str());
        GuiLabel(bounds, buttonText);
        UiElement::draw(deltaTime);
    }

    void NodeUiElement::update(const float deltaTime)
    {
        updateDraggable(this);
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

    NodeUiElement *NodeUiElement::copy()
    {
        auto *copyElement = &UiPool::nodeUiElementPool.get().setup(id.append("_copy"), parent, engineNode);
        copyElement->setAnchor(anchor);
        copyElement->setBounds(localPosition, localSize);
        return copyElement;
    }

    void NodeUiElement::switchMuteState()
    {
        if (isMuted)
        {
            isMuted = false;
            localState = localStateBeforeMuted;
        }
        else
        {
            localStateBeforeMuted = localState;
            localState = STATE_DISABLED;
            isMuted = true;
        }
    }

    GuiState NodeUiElement::getState() const
    {
        if (parentNode != nullptr)
        {
            auto parentState = parentNode->state;
            return std::max(localState, parentState);
        }

        return state;
    }

    bool NodeUiElement::tryDeleteSelf()
    {
        UiPool::nodeUiElementPool.release(*this);
        return true;
    }
} // BreadEditor
