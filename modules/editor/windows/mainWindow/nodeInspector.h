#pragma once
#include "action.h"
#include "node.h"
#include "nodeInstance.h"
#include "uitoolkit/uiPanel.h"
using namespace BreadEngine;

namespace BreadEditor
{
    class NodeInspector final : public UiElement
    {
    public:
        explicit NodeInspector(const std::string &id);

        explicit NodeInspector(const std::string &id, UiElement *parentElement);

        ~NodeInspector() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        void deleteSelf() override;

    private:
        const char *title = "Node Inspector";
        std::vector<SubscriptionHandle> subscriptionHandles;
        std::vector<NodeInstance*> nodeInstances;
        Vector2 scrollPos = {0.0f, 0.0f};
        Rectangle scrollView = {0.0f, 0.0f, 0.0f, 0.0f};

        NodeInstance *findNodeInstanceByEngineNode(const Node *node) const;

        void subscribe();

        void unsubscribe();

        void onNodeCreated(Node *node);

        void onNodeChangedParent(Node *node);

        void onNodeRemoved(const Node *node);

        void updateScrollView(Rectangle lastNodeBounds);
        void drawNodes() const;
        void drawLines() const;
    };
} // BreadEditor
