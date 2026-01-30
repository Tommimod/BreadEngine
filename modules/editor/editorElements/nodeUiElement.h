#pragma once
#include "action.h"
#include "node.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeUiElement final : public UiElement, public IUiDraggable
    {
    public:
        Action<NodeUiElement *> onSelected;

        NodeUiElement();

        ~NodeUiElement() override;

        NodeUiElement &setup(const std::string &id, UiElement *parentElement, Node *node);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        [[nodiscard]] Node *getNode() const;

        void setParentNode(NodeUiElement *nextParentNode);

        void setEngineNode(Node *nextEngineNode);

        void setState(GuiState nextState) override;

        NodeUiElement *copy();

        void switchMuteState();

        [[nodiscard]] GuiState getState() const;

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isMuted = false;
        bool _isPreparedToClick = false;
        GuiState _localStateBeforeMuted = STATE_NORMAL;
        GuiState _localState = STATE_NORMAL;
        Node *_engineNode = nullptr;
        NodeUiElement *_parentNode = nullptr;

        void prepareToClick();
    };
} // BreadEditor
