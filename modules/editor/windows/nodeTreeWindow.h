#pragma once
#include "action.h"
#include "node.h"
#include "../editorElements/nodeUiElement.h"
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

        void onFrameEnd(float deltaTime) override;

        void dispose() override;

        const char *getTitle() override { return _title; }

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = Id.c_str();
        std::vector<SubscriptionHandle> _nodeNotificatorSubscriptions{};
        std::vector<NodeUiElement *> _nodeUiElements{};
        NodeUiElement *_draggedNodeUiElementCopy = nullptr;

        [[nodiscard]] NodeUiElement *getNodeUiElementByEngineNode(const Node *node) const;

        void rebuild();

        void rebuildByNode(Node *node);

        void onNodeCreated(Node *node);

        void onNodeChangedParent(const Node *node);

        void onNodeChangedActive(const Node *node) const;

        void onNodeRemoved(const Node *node);

        void onNodeSelected(NodeUiElement *nodeUiElement);

        void onElementStartDrag(UiElement *uiElement);

        void onElementEndDrag(UiElement *uiElement);

        void recalculateUiNodes(Node &startNode, int &nodeOrder, bool isParentExpanded = true);

        void drawLines(Node &startNode, bool isParentExpanded = true) const;
    };
} // BreadEditor
