#pragma once
#include "node.h"
#include "uitoolkit/uiElement.h"
using namespace BreadEngine;

namespace BreadEditor
{
    class NodeInstance final : public UiElement
    {
    public:
        NodeInstance();

        ~NodeInstance() override;

        NodeInstance &setup(const std::string &id, UiElement *parentElement, Node *node);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        [[nodiscard]] Node *getNode() const;

        void setParentNode(NodeInstance *nextParentNode);

        void setEngineNode(Node *nextEngineNode);

    protected:
        void deleteSelf() override;

    private:
        Node *engineNode = nullptr;
        NodeInstance *parentNode = nullptr;
    };
} // BreadEditor
