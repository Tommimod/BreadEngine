#include "nodeInspector.h"

#include "engine.h"
#include "nodeNotificator.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    NodeInspector::NodeInspector(const std::string &id)
    {
        setup(id);
        subscribe();
    }

    NodeInspector::NodeInspector(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
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
        auto &element = UiPool::nodeInstancePool.get().setup(id, this, node);

        element.setParentNode(parentNode);
        element.setAnchor(UI_LEFT_TOP);
        const auto horizontalSize = element.getSizeInPixByPercent({elementWidthInPercent, 0}).x;
        element.setSize({horizontalSize, elementHeight});

        nodeUiElements.emplace_back(&element);
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
    }

    void NodeInspector::onNodeChangedParent(Node *node)
    {
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
    }

    void NodeInspector::onNodeRemoved(const Node *node)
    {
        const auto instance = findNodeUiElementByEngineNode(node);
        destroyChild(instance);
        nodeUiElements.erase(ranges::find(nodeUiElements, instance));
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
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

    void NodeInspector::recalculateUiNodes(Node &startNode, int& nodeOrder) const
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
        constexpr auto lineThickness = 1.0f;
        constexpr float nodeHorizontalPadding = 7.5f;
        constexpr float nodeVerticalPadding = 5.0f;

        for (const auto &instance: nodeUiElements)
        {
            const auto bounds = instance->getBounds();
            auto leftCenterPoint = Vector2{bounds.x, bounds.y + bounds.height * .5f};
            auto targetPoint = Vector2(leftCenterPoint);
            targetPoint.x -= nodeHorizontalPadding;

            DrawLineEx(leftCenterPoint, targetPoint, lineThickness, lineColor);

            const bool isLast = nodeUiElements.back() == instance;
            int childCount = instance->getNode()->getChildCount();
            if (isLast || childCount == 0)
            {
                continue;
            }

            auto verticalTargetPoint = Vector2(targetPoint);
            verticalTargetPoint.y += (nodeVerticalPadding + bounds.height) * static_cast<float>(childCount);
            DrawLineEx(targetPoint, verticalTargetPoint, lineThickness, lineColor);
        }
    }
} // BreadEditor
