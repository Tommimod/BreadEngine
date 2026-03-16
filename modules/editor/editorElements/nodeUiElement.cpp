#include "nodeUiElement.h"
#include "editor.h"
#include "engine.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeUiElement::NodeUiElement() = default;

    NodeUiElement::~NodeUiElement() = default;

    NodeUiElement &NodeUiElement::setup(const std::string_view &id, UiElement *parentElement, Node *node)
    {
        this->_engineNode = node;
        _isRootNode = _engineNode->getParent() == nullptr;
        _expandButton = &UiPool::buttonPool.get().setup(TextFormat("%s_expandButton", id), this, "");
        _expandButton->setAnchor(UI_LEFT_TOP);
        _expandButton->setPivot({1, 0});
        _expandButton->setPosition({-2, 5});
        _expandButton->setClickOutside(true);
        _expandButton->onClick.subscribe([this](UiButton *)
        {
            _isExpanded = !_isExpanded;
            updateExpandButtonText();
            onExpandStateChanged.invoke(this);
        });
        updateExpandButtonText();
        initializeOptionsOwner(this, _isRootNode ? std::vector{_options[0], _options[1], _options[2]} : _options);
        UiElement::setup(id, parentElement);
        return *this;
    }

    void NodeUiElement::awake()
    {
        const auto size = getSize().y * .5f;
        _expandButton->setSize({size, size});
    }

    void NodeUiElement::draw(const float deltaTime)
    {
        if (GuiButton(getBounds(), nullptr))
        {
            onSelected.invoke(this);
        }

        const auto buttonText = GuiIconText(ICON_CPU, _engineNode->getName().c_str());
        GuiLabel(getBounds(), buttonText);
    }

    void NodeUiElement::update(const float deltaTime)
    {
        const auto childCount = _engineNode->getChildCount();
        _expandButton->isActive = childCount > 0;
        if (childCount == 0)
        {
            _isExpanded = true;
        }

        updateOptionsOwner();
        updateDraggable(this);
    }

    void NodeUiElement::dispose()
    {
        _isRootNode = false;
        _isExpanded = true;
        _isMuted = false;
        _localStateBeforeMuted = STATE_NORMAL;
        _localState = STATE_NORMAL;
        _engineNode = nullptr;
        _parentNode = nullptr;
        _expandButton = nullptr;
        onCopyRequested.unsubscribeAll();
        onDeleteRequested.unsubscribeAll();
        onDuplicateRequested.unsubscribeAll();
        onExpandStateChanged.unsubscribeAll();
        onPasteRequested.unsubscribeAll();
        onSelected.unsubscribeAll();
        disposeDraggable();
        disposeOptionsOwner();
        UiElement::dispose();
    }

    Node *NodeUiElement::getNode() const
    {
        return _engineNode;
    }

    void NodeUiElement::setParentNode(NodeUiElement *nextParentNode)
    {
        _parentNode = nextParentNode;
    }

    void NodeUiElement::setEngineNode(Node *nextEngineNode)
    {
        _engineNode = nextEngineNode;
    }

    void NodeUiElement::setIsSelected(const bool isSelected)
    {
        if (_localState == STATE_DISABLED) return;
        setState(isSelected ? STATE_FOCUSED : STATE_NORMAL);
    }

    void NodeUiElement::setState(const GuiState nextState)
    {
        UiElement::setState(nextState);
        _localState = nextState;
    }

    NodeUiElement *NodeUiElement::copySingle() const
    {
        auto *copyElement = &UiPool::nodeUiElementPool.get().setup(id + "_copy", _parent, _engineNode);
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
            const auto parentState = _parentNode->_state;
            return std::max(_localState, parentState);
        }

        return _state;
    }

    bool NodeUiElement::tryDeleteSelf()
    {
        UiPool::nodeUiElementPool.release(*this);
        return true;
    }

    void NodeUiElement::handleSelectedOption(const int index)
    {
        if (index == 1) // Create empty
        {
            Engine::nodePool.get().setup("Empty Node", *_engineNode);
        }
        else if (index == 2) // Copy
        {
            onCopyRequested.invoke(Node::getDataForCopy(*_engineNode));
        }
        else if (index == 3) // Paste
        {
            onPasteRequested.invoke(this);
        }
        else if (index == 4) // Duplicate
        {
            Node::createCopyFromNode(*_engineNode);
        }
        else if (index == 5) // Delete
        {
            _engineNode->destroy();
        }
    }

    void NodeUiElement::updateExpandButtonText() const
    {
        _expandButton->setText(_isExpanded ? "-" : "+");
    }
} // BreadEditor
