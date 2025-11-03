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
        static std::string Id;

        explicit NodeTree(const std::string &id);

        explicit NodeTree(const std::string &id, UiElement *parentElement);

        ~NodeTree() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    private:
        const char *title = "Node Inspector";
        std::vector<SubscriptionHandle> nodeNotificatorSubscriptions;
        std::vector<NodeUiElement *> nodeUiElements;
        Vector2 scrollPos = {0.0f, 0.0f};
        Rectangle contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        Rectangle scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        NodeUiElement *selectedNodeUiElement = nullptr;
        NodeUiElement *draggedNodeUiElementCopy = nullptr;

        NodeUiElement *findNodeUiElementByEngineNode(const Node *node) const;

        void subscribe();

        void unsubscribe();

        void onNodeCreated(Node *node);

        void onNodeChangedParent(Node *node);

        void onNodeChangedActive(Node *node);

        void onNodeRemoved(const Node *node);

        void onNodeSelected(NodeUiElement *nodeUiElement);

        void onElementStartDrag(UiElement *uiElement);

        void onElementEndDrag(UiElement *uiElement);

        void updateScrollView(Rectangle lastNodeBounds);

        void recalculateUiNodes(Node &startNode, int &nodeOrder) const;

        void drawLines(Node &startNode) const;
    };
} // BreadEditor
