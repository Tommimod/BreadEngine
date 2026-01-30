#include "nodeUiElement.h"
#include "editor.h"
#include "engine.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeUiElement::NodeUiElement() = default;

    NodeUiElement::~NodeUiElement() = default;

    NodeUiElement &NodeUiElement::setup(const std::string &id, UiElement *parentElement, Node *node)
    {
        this->_engineNode = node;
        return dynamic_cast<NodeUiElement &>(UiElement::setup(id, parentElement));
    }

    void NodeUiElement::draw(const float deltaTime)
    {
        prepareToClick();
        GuiButton(getBounds(), nullptr);
        const auto mousePos = GetMousePosition();
        if (_isPreparedToClick && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Engine::isCollisionPointRec(mousePos, getBounds()))
        {
            auto parentBounds = _parent->getBounds();
            parentBounds.height -= RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
            parentBounds.y += RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
            const auto isNeedIgnore = !Engine::isCollisionPointRec(mousePos, parentBounds);
            if (!isNeedIgnore)
            {
                onSelected.invoke(this);
            }
        }

        const auto buttonText = GuiIconText(ICON_CPU, _engineNode->getName().c_str());
        GuiLabel(getBounds(), buttonText);
    }

    void NodeUiElement::update(const float deltaTime)
    {
        updateDraggable(this);
    }

    void NodeUiElement::dispose()
    {
        UiElement::dispose();
    }

    Node *NodeUiElement::getNode() const
    {
        return _engineNode;
    }

    void NodeUiElement::setParentNode(NodeUiElement *nextParentNode)
    {
        this->_parentNode = nextParentNode;
    }

    void NodeUiElement::setEngineNode(Node *nextEngineNode)
    {
        this->_engineNode = nextEngineNode;
    }

    void NodeUiElement::setState(GuiState nextState)
    {
        UiElement::setState(nextState);
        _localState = nextState;
    }

    NodeUiElement *NodeUiElement::copy()
    {
        auto *copyElement = &UiPool::nodeUiElementPool.get().setup(id.append("_copy"), _parent, _engineNode);
        copyElement->setAnchor(_anchor);
        copyElement->setBounds(_localPosition, _localSize);
        return copyElement;
    }

    void NodeUiElement::switchMuteState()
    {
        if (_isMuted)
        {
            _isMuted = false;
            _localState = _localStateBeforeMuted;
        }
        else
        {
            _localStateBeforeMuted = _localState;
            _localState = STATE_DISABLED;
            _isMuted = true;
        }
    }

    GuiState NodeUiElement::getState() const
    {
        if (_parentNode != nullptr)
        {
            auto parentState = _parentNode->_state;
            return std::max(_localState, parentState);
        }

        return _state;
    }

    bool NodeUiElement::tryDeleteSelf()
    {
        UiPool::nodeUiElementPool.release(*this);
        return true;
    }

    void NodeUiElement::prepareToClick()
    {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _isPreparedToClick = Engine::isCollisionPointRec(GetMousePosition(), _bounds);
        }
    }
} // BreadEditor
