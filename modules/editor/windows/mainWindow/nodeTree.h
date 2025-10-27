#pragma once
#include "action.h"
#include "node.h"
#include "nodeUiElement.h"
#include "uitoolkit/IUiResizable.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeTree final : public UiElement, IUiResizable
    {
    public:
        explicit NodeTree(const std::string &id);

        explicit NodeTree(const std::string &id, UiElement *parentElement);

        ~NodeTree() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        void deleteSelf() override;

    private:
        const char *title = "Node Inspector";
        std::vector<SubscriptionHandle> subscriptionHandles;
        std::vector<NodeUiElement *> nodeUiElements;
        Vector2 scrollPos = {0.0f, 0.0f};
        Rectangle scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        NodeUiElement *selectedNodeUiElement = nullptr;
        std::unordered_map<NodeUiElement *, SubscriptionHandle> nodeUiElementSubscriptions;

        NodeUiElement *findNodeUiElementByEngineNode(const Node *node) const;

        void subscribe();

        void unsubscribe();

        void onNodeCreated(Node *node);

        void onNodeChangedParent();

        void onNodeChangedActive(Node *node);

        void onNodeRemoved(const Node *node);

        void onNodeSelected(NodeUiElement *nodeUiElement);

        void updateScrollView(Rectangle lastNodeBounds);

        void recalculateUiNodes(Node &startNode, int &nodeOrder) const;

        void drawLines(Node &startNode) const;
    };
} // BreadEditor
