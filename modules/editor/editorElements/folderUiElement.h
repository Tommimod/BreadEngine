#pragma once
#include "action.h"
#include "nodeUiElement.h"
#include "systems/fileSystem.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/uiButton.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiLabelButton.h"

namespace BreadEditor {
    class FolderUiElement : public UiElement, public IUiDraggable
    {
    public:
        static constexpr auto elementIdFormat = "FoldInsT_%s_%d";
        Action<FolderUiElement *> onExpandStateChanged;

        FolderUiElement();

        ~FolderUiElement() override;

        [[nodiscard]] FolderUiElement &setup(const std::string &id, UiElement *parentElement, Folder *folder);

        [[nodiscard]] Folder &getFolder() const { return *_engineFolder; }

        [[nodiscard]] bool getIsExpanded() const { return _isExpanded; }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isExpanded = false;
        Folder *_engineFolder = nullptr;
        UiLabelButton &_button;
        UiButton &_expandButton;

        void updateExpandButtonText() const;
    };
} // BreadEditor
