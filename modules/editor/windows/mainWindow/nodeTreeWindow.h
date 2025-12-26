#pragma once
#include "action.h"
#include "node.h"
#include "nodeUiElement.h"
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiWindow.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeTreeWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit NodeTreeWindow(const std::string &id);

        explicit NodeTreeWindow(const std::string &id, UiElement *parentElement);

        ~NodeTreeWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = "Node Tree";
        std::vector<SubscriptionHandle> _nodeNotificatorSubscriptions {};
        std::vector<NodeUiElement *> _nodeUiElements {};
        NodeUiElement *_draggedNodeUiElementCopy = nullptr;

        NodeUiElement *findNodeUiElementByEngineNode(const Node *node) const;

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
