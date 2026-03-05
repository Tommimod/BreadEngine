#pragma once
#include "action.h"
#include "../../engine/configs/assetsConfig.h"
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

        [[nodiscard]] FolderUiElement &setup(const std::string &id, UiElement *parentElement, const std::string &folderGuid);

        [[nodiscard]] const std::string &getFolderGuid() const { return _folderGuid; }

        [[nodiscard]] bool getIsExpanded() const { return _isExpanded; }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        FolderUiElement *copy();

    protected:
        bool tryDeleteSelf() override;

    private:
        AssetsConfig &_assetConfig;
        bool _isExpanded = false;
        Folder *_engineFolder = nullptr;
        UiLabelButton *_button = nullptr;
        UiButton *_expandButton = nullptr;
        std::string _folderGuid;

        void updateExpandButtonText() const;
    };
} // BreadEditor
