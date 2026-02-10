#pragma once
#include "action.h"
#include "systems/fileSystem.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiLabelButton.h"

namespace BreadEditor {
    class FolderUiElement : public UiElement, public IUiDraggable
    {
    public:
        BreadEngine::Action<FolderUiElement *> onExpanded;

        FolderUiElement();

        ~FolderUiElement() override;

        [[nodiscard]] FolderUiElement &setup(const std::string &id, UiElement *parentElement, BreadEngine::Folder *folder);

        [[nodiscard]] FolderUiElement &setup(const std::string &id, FolderUiElement *parentElement, BreadEngine::Folder *folder);

        [[nodiscard]] bool &IsExpanded() { return _isExpanded; }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isExpanded = false;
        BreadEngine::Folder *_engineFolder = nullptr;
        FolderUiElement *_parentFolderElement = nullptr;
        UiLabelButton &_button;
    };
} // BreadEditor
