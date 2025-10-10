#include "nodeInspector.h"
#include "nodeNotificator.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor
{
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
        drawNodes();
        drawLines();
        UiElement::draw(deltaTime);
    }

    void NodeInspector::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
        if (scrollView.width == 0 && scrollView.height == 0)
        {
            scrollView = bounds;
        }

        updateScrollView(nodeInstances.back()->getBounds());
    }

    NodeInstance *NodeInspector::findNodeInstanceByEngineNode(const Node *node) const
    {
        for (const auto child: childs)
        {
            if (const auto nodeInstance = dynamic_cast<NodeInstance *>(child); nodeInstance && nodeInstance->getNode() == node)
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
        constexpr auto nodeInstanceIdFormat = "NinsT_%d";
        constexpr float nodeHeight = 20.0f;
        constexpr float nodeWidthInPercent = 0.8f;

        const auto parentNode = findNodeInstanceByEngineNode(node);
        const auto id = TextFormat(nodeInstanceIdFormat, static_cast<int>(childs.size()));
        auto &instance = UiPool::nodeInstancePool.get().setup(id, this, node);

        instance.setParentNode(parentNode);
        instance.setAnchor(UI_LEFT_TOP);
        const auto horizontalSize = instance.getSizeInPixByPercent({nodeWidthInPercent, 0}).x;
        instance.setSize({horizontalSize, nodeHeight});
        nodeInstances.emplace_back(&instance);
    }

    void NodeInspector::onNodeChangedParent(Node *node)
    {
    }

    void NodeInspector::onNodeRemoved(const Node *node)
    {
        const auto instance = findNodeInstanceByEngineNode(node);
        destroyChild(instance);
        nodeInstances.erase(ranges::find(nodeInstances, instance));
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

    void NodeInspector::drawNodes() const
    {
        for (const auto instance: nodeInstances)
        {
            constexpr float nodeHorizontalPadding = 15.0f;
            constexpr float nodeVerticalPadding = 5.0f;

            const auto node = instance->getNode();
            const auto nodeHeight = instance->getSize().y;
            const auto deepLevel = static_cast<float>(node->getDeepLevel());

            const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
            const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(node->getChildCount() - 1);
            instance->setPosition({nodeHorizontalPadding + horizontalOffset, 25 + verticalPadding});
        }
    }

    void NodeInspector::drawLines() const
    {
        constexpr auto lineColor = BLACK;
        constexpr auto lineThickness = 1.1f;
        constexpr float nodeHorizontalPadding = 7.5f;
        constexpr float nodeVerticalPadding = 5.0f;

        for (const auto &instance: nodeInstances)
        {
            const auto bounds = instance->getBounds();
            auto leftCenterPoint = Vector2{bounds.x, bounds.y + bounds.height * .5f};
            auto targetPoint = Vector2(leftCenterPoint);
            targetPoint.x -= nodeHorizontalPadding;

            DrawLineEx(leftCenterPoint, targetPoint, lineThickness, lineColor);

            const bool isLast = nodeInstances.back() == instance;
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
