#include "nodeInspector.h"

#include "engine.h"
#include "nodeNotificator.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeInspector::NodeInspector(const std::string &id)
    {
        setup(id);
        isHorizontalResized = true;
        subscribe();
    }

    NodeInspector::NodeInspector(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        isHorizontalResized = true;
        subscribe();
    }

    NodeInspector::~NodeInspector()
    {
        unsubscribe();
    }

    void NodeInspector::draw(const float deltaTime)
    {
        GuiScrollPanel(bounds, title, scrollView, &scrollPos, &scrollView);
        drawLines(Engine::getRootNode());
        UiElement::draw(deltaTime);
    }

    void NodeInspector::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
        if (scrollView.width == 0 && scrollView.height == 0)
        {
            scrollView = bounds;
        }

        updateScrollView(nodeUiElements.back()->getBounds());
        updateResizable(*this);
    }

    NodeUiElement *NodeInspector::findNodeUiElementByEngineNode(const Node *node) const
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

    void NodeInspector::deleteSelf()
    {
    }

    void NodeInspector::subscribe()
    {
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeCreated.subscribe([this](Node *node) { this->onNodeCreated(node); }));
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeChangedParent.subscribe([this](Node *node) { this->onNodeChangedParent(node); }));
        subscriptionHandles.emplace_back(
            NodeNotificator::onNodeDestroyed.subscribe([this](Node *node) { this->onNodeRemoved(node); }));
    }

    void NodeInspector::unsubscribe()
    {
        NodeNotificator::onNodeCreated.unsubscribe(subscriptionHandles[0]);
        NodeNotificator::onNodeChangedParent.unsubscribe(subscriptionHandles[1]);
        NodeNotificator::onNodeDestroyed.unsubscribe(subscriptionHandles[2]);
        subscriptionHandles.clear();
    }

    void NodeInspector::onNodeCreated(Node *node)
    {
        constexpr auto elementIdFormat = "NinsT_%d";
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        const auto parentNode = findNodeUiElementByEngineNode(node);
        const auto id = TextFormat(elementIdFormat, static_cast<int>(childs.size()));
        auto &element = UiPool::nodeUiElementPool.get().setup(id, this, node);

        element.setParentNode(parentNode);
        element.setAnchor(UI_LEFT_TOP);
        const auto horizontalSize = element.getSizeInPixByPercent({elementWidthInPercent, 0}).x;
        element.setSize({horizontalSize, elementHeight});

        nodeUiElements.emplace_back(&element);
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
        auto handler = element.onSelected.subscribe([this](NodeUiElement *nodeUiElement) { this->onNodeSelected(nodeUiElement); });
        nodeUiElementSubscriptions.emplace(&element, handler);
    }

    void NodeInspector::onNodeChangedParent(Node *node)
    {
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
    }

    void NodeInspector::onNodeRemoved(const Node *node)
    {
        const auto instance = findNodeUiElementByEngineNode(node);
        instance->onSelected.unsubscribe(nodeUiElementSubscriptions[instance]);
        nodeUiElementSubscriptions.erase(instance);
        destroyChild(instance);
        nodeUiElements.erase(ranges::find(nodeUiElements, instance));
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
    }

    void NodeInspector::onNodeSelected(NodeUiElement *nodeUiElement)
    {
        selectedNodeUiElement = nodeUiElement;
    }

    void NodeInspector::updateScrollView(const Rectangle lastNodeBounds)
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

    void NodeInspector::recalculateUiNodes(Node &startNode, int &nodeOrder) const
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

    void NodeInspector::drawLines(Node &startNode) const
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
