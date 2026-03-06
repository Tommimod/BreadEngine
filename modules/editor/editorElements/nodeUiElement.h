#pragma once
#include "action.h"
#include "node.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/IUiOptionsOwner.h"
#include "uitoolkit/uiButton.h"
#include "uitoolkit/uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeUiElement final : public UiElement, public IUiDraggable, public IUiOptionsOwner
    {
    public:
        static constexpr auto elementIdFormat = "NodeInsT_%s_%d";

        Action<NodeUiElement *> onSelected;
        Action<NodeUiElement *> onExpandStateChanged;
        Action<NodeUiElement *> onDuplicateRequested;
        Action<NodeUiElement *> onCopyRequested;
        Action<NodeUiElement *> onPasteRequested;
        Action<NodeUiElement *> onDeleteRequested;

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

        void setIsSelected(bool isSelected);

        void setState(GuiState nextState) override;

        NodeUiElement *copySingle();

        void switchMuteState();

        [[nodiscard]] GuiState getState() const;

        [[nodiscard]] bool &getIsExpanded() { return _isExpanded; }

    protected:
        bool tryDeleteSelf() override;

        void handleSelectedOption(int index) override;

    private:
        Node *_engineNode = nullptr;
        NodeUiElement *_parentNode = nullptr;
        const std::vector<std::string> _options{"Copy", "Paste", "Duplicate", "Delete"};
        UiButton &_expandButton;
        GuiState _localStateBeforeMuted = STATE_NORMAL;
        GuiState _localState = STATE_NORMAL;
        bool _isExpanded = true;
        bool _isMuted = false;
        bool _isPreparedToClick = false;
        bool _isRootNode = false;

        void prepareToClick();

        void updateExpandButtonText() const;
    };
} // BreadEditor
