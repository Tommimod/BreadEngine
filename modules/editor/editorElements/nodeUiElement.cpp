#include "nodeUiElement.h"
#include "editor.h"
#include "engine.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeUiElement::NodeUiElement() : _expandButton(UiPool::buttonPool.get())
    {
        _expandButton.setup(id + "expandButton", this, "");
    }

    NodeUiElement::~NodeUiElement() = default;

    NodeUiElement &NodeUiElement::setup(const std::string &id, UiElement *parentElement, Node *node)
    {
        this->_engineNode = node;
        _isRootNode = _engineNode->getParent() == nullptr;
        updateExpandButtonText();
        _expandButton.setAnchor(UI_LEFT_TOP);
        _expandButton.setPivot({1, 0});
        _expandButton.setPosition({-2, 5});
        _expandButton.setClickOutside(true);
        _expandButton.onClick.subscribe([this](UiButton *)
        {
            _isExpanded = !_isExpanded;
            updateExpandButtonText();
            onExpandStateChanged.invoke(this);
        });

        initializeOptionsOwner(this, _isRootNode ? std::vector{_options[0], _options[1]} : _options);
        UiElement::setup(id, parentElement);
        return *this;
    }

    void NodeUiElement::awake()
    {
        const auto size = getSize().y * .5f;
        _expandButton.setSize({size, size});
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
        _expandButton.isActive = childCount > 0;
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
        _expandButton.onClick.unsubscribeAll();
        onCopyRequested.unsubscribeAll();
        onDeleteRequested.unsubscribeAll();
        onDuplicateRequested.unsubscribeAll();
        onExpandStateChanged.unsubscribeAll();
        onPasteRequested.unsubscribeAll();
        onSelected.unsubscribeAll();
        disposeDraggable();
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

    NodeUiElement *NodeUiElement::copySingle()
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
        if (index == 1) //Copy
        {
        }
        else if (index == 2) //Paste
        {
        }
        else if (index == 3) //Duplicate
        {
        }
        else if (index == 4) //Delete
        {
        }
    }

    void NodeUiElement::updateExpandButtonText() const
    {
        _expandButton.setText(_isExpanded ? "-" : "+");
    }
} // BreadEditor
