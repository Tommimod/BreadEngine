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

        NodeUiElement *getSelectedNodeUiElement() const;

    private:
        const char *_title = "Node Inspector";
        std::vector<SubscriptionHandle> _nodeNotificatorSubscriptions;
        std::vector<NodeUiElement *> _nodeUiElements;
        Vector2 _scrollPos = {0.0f, 0.0f};
        Rectangle _contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        Rectangle _scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        NodeUiElement *_selectedNodeUiElement = nullptr;
        NodeUiElement *_draggedNodeUiElementCopy = nullptr;

        NodeUiElement *findNodeUiElementByEngineNode(const Node *node) const;

        void subscribe();

        void unsubscribe();

        void onNodeCreated(Node *node);

        void onNodeChangedParent(const Node *node) const;

        void onNodeChangedActive(const Node *node) const;

        void onNodeRemoved(const Node *node);

        void onNodeSelected(NodeUiElement *nodeUiElement);

        void onElementStartDrag(UiElement *uiElement);

        void onElementEndDrag(UiElement *uiElement);

        void updateScrollView(Rectangle lastNodeBounds);

        void recalculateUiNodes(Node &startNode, int &nodeOrder) const;

        void drawLines(Node &startNode) const;
    };
} // BreadEditor
