#pragma once
#include "action.h"
#include "node.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/uiButton.h"
#include "uitoolkit/uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeUiElement final : public UiElement, public IUiDraggable
    {
    public:
        static constexpr auto elementIdFormat = "NodeInsT_%s_%d";

        Action<NodeUiElement *> onSelected;
        Action<NodeUiElement *> onExpandStateChanged;

        NodeUiElement();

        ~NodeUiElement() override;

        NodeUiElement &setup(const std::string &id, UiElement *parentElement, Node *node);

        void awake() override;

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

        [[nodiscard]] bool &getIsExpanded() { return _isExpanded; }

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isExpanded = true;
        bool _isMuted = false;
        bool _isPreparedToClick = false;
        GuiState _localStateBeforeMuted = STATE_NORMAL;
        GuiState _localState = STATE_NORMAL;
        Node *_engineNode = nullptr;
        NodeUiElement *_parentNode = nullptr;
        UiButton &_expandButton;

        void prepareToClick();

        void updateExpandButtonText() const;
    };
} // BreadEditor
