#include "nodeTree.h"

#include "editor.h"
#include "engine.h"
#include "nodeNotificator.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeTree::NodeTree(const std::string &id)
    {
        setup(id);
        isHorizontalResized = true;
        subscribe();
    }

    NodeTree::NodeTree(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        isHorizontalResized = true;
        subscribe();
    }

    NodeTree::~NodeTree()
    {
        unsubscribe();
    }

    void NodeTree::draw(const float deltaTime)
    {
        GuiSetState(state);
        GuiScrollPanel(bounds, title, scrollView, &scrollPos, &scrollView);
        drawLines(Engine::getRootNode());
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void NodeTree::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
        if (scrollView.width == 0 && scrollView.height == 0)
        {
            scrollView = bounds;
        }

        updateScrollView(nodeUiElements.back()->getBounds());
        updateResizable(*this);
    }

    NodeUiElement *NodeTree::findNodeUiElementByEngineNode(const Node *node) const
    {
        for (const auto child: childs)
        {
            if (const auto nodeInstance = dynamic_cast<NodeUiElement *>(child); nodeInstance && nodeInstance->getNode() == node)
            {
                return nodeInstance;
            }
        }

        return nullptr;
    }

    void NodeTree::subscribe()
    {
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeCreated.subscribe([this](Node *node) { this->onNodeCreated(node); }));
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeChangedParent.subscribe([this](Node *) { this->onNodeChangedParent(); }));
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeDestroyed.subscribe([this](Node *node) { this->onNodeRemoved(node); }));
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeChangedActive.subscribe([this](Node *node) { this->onNodeChangedActive(node); }));
    }

    void NodeTree::unsubscribe()
    {
        NodeNotificator::onNodeCreated.unsubscribe(subscriptionHandles[0]);
        NodeNotificator::onNodeChangedParent.unsubscribe(subscriptionHandles[1]);
        NodeNotificator::onNodeDestroyed.unsubscribe(subscriptionHandles[2]);
        NodeNotificator::onNodeChangedActive.unsubscribe(subscriptionHandles[3]);
        subscriptionHandles.clear();
    }

    void NodeTree::onNodeCreated(Node *node)
    {
        constexpr auto elementIdFormat = "NinsT_%d";
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        const auto id = TextFormat(elementIdFormat, static_cast<int>(childs.size()));
        auto &element = UiPool::nodeUiElementPool.get().setup(id, this, node);

        element.setParentNode(findNodeUiElementByEngineNode(node->getParent()));
        element.setAnchor(UI_LEFT_TOP);
        element.setSize({0, elementHeight});
        element.setSizePercentPermanent({elementWidthInPercent, -1});

        nodeUiElements.emplace_back(&element);
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
        auto handler = element.onSelected.subscribe([this](NodeUiElement *nodeUiElement) { this->onNodeSelected(nodeUiElement); });
        nodeUiElementSubscriptions.emplace(&element, handler);
    }

    void NodeTree::onNodeChangedParent()
    {
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
    }

    void NodeTree::onNodeChangedActive(Node *node)
    {
        const auto instance = findNodeUiElementByEngineNode(node);
        instance->setState(node->getIsActive() ? STATE_NORMAL : STATE_DISABLED);
    }

    void NodeTree::onNodeRemoved(const Node *node)
    {
        const auto instance = findNodeUiElementByEngineNode(node);
        instance->onSelected.unsubscribe(nodeUiElementSubscriptions[instance]);
        nodeUiElementSubscriptions.erase(instance);
        destroyChild(instance);
        nodeUiElements.erase(ranges::find(nodeUiElements, instance));
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
    }

    void NodeTree::onNodeSelected(NodeUiElement *nodeUiElement)
    {
        selectedNodeUiElement = nodeUiElement;
        Node *node = nullptr;
        if (selectedNodeUiElement != nullptr)
        {
            node = selectedNodeUiElement->getNode();
        }

        Editor::GetInstance().main_window.getNodeInspector().lookupNode(node);
    }

    void NodeTree::updateScrollView(const Rectangle lastNodeBounds)
    {
        if (scrollView.x < lastNodeBounds.x)
        {
            scrollView.x = lastNodeBounds.x;
        }

        if (scrollView.y < lastNodeBounds.y)
        {
            scrollView.y = lastNodeBounds.y;
        }
    }

    void NodeTree::recalculateUiNodes(Node &startNode, int &nodeOrder) const
    {
        const auto element = findNodeUiElementByEngineNode(&startNode);
        constexpr float nodeHorizontalPadding = 15.0f;
        constexpr float nodeVerticalPadding = 5.0f;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(startNode.getDeepLevel());

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, 25 + verticalPadding});
        nodeOrder++;
        if (startNode.getChildCount() == 0)
        {
            return;
        }

        for (const auto child: startNode.getAllChilds())
        {
            recalculateUiNodes(*child, nodeOrder);
        }
    }

    void NodeTree::drawLines(Node &startNode) const
    {
        constexpr auto lineColor = BLACK;
        constexpr float nodeHorizontalPadding = 15.0f * .5f;

        const auto instance = findNodeUiElementByEngineNode(&startNode);
        const bool isLast = nodeUiElements.back() == instance;
        int childCount = instance->getNode()->getChildCount();
        if (isLast || childCount == 0)
        {
            return;
        }

        const auto bounds = instance->getBounds();
        auto leftCenterPoint = Vector2{bounds.x, bounds.y + bounds.height * .5f};
        auto targetPoint = Vector2(leftCenterPoint);
        targetPoint.x -= nodeHorizontalPadding;
        DrawLineV(leftCenterPoint, targetPoint, lineColor);

        const auto childs = startNode.getAllChilds();
        const auto lastChild = childs.back();
        const auto lastElement = findNodeUiElementByEngineNode(lastChild);
        auto lastTargetPoint = Vector2(targetPoint);
        lastTargetPoint.y = 1 + lastElement->getBounds().y + lastElement->getBounds().height * .5f;
        targetPoint.y++;
        DrawLineV(targetPoint, lastTargetPoint, lineColor);

        for (const auto child: childs)
        {
            const auto childElement = findNodeUiElementByEngineNode(child);
            const auto childBound = childElement->getBounds();
            const auto xPoint = childBound.x;
            const auto yPoint = childBound.y + childBound.height * .5f;
            const auto target = Vector2(xPoint, yPoint);
            const auto from = Vector2{targetPoint.x, yPoint};
            DrawLineV(from, target, lineColor);
        }

        for (const auto child: childs)
        {
            drawLines(*child);
        }
    }
} // BreadEditor
