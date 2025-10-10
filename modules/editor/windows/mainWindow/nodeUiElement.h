#pragma once
#include "node.h"
#include "uitoolkit/uiElement.h"
using namespace BreadEngine;

namespace BreadEditor
{
    class NodeUiElement final : public UiElement
    {
    public:
        NodeUiElement();

        ~NodeUiElement() override;

        NodeUiElement &setup(const std::string &id, UiElement *parentElement, Node *node);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        [[nodiscard]] Node *getNode() const;

        void setParentNode(NodeUiElement *nextParentNode);

        void setEngineNode(Node *nextEngineNode);

    protected:
        void deleteSelf() override;

    private:
        Node *engineNode = nullptr;
        NodeUiElement *parentNode = nullptr;
    };
} // BreadEditor
