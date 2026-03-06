#pragma once
#include "editorElements/fileUiElement.h"
#include "editorElements/folderUiElement.h"
#include "../../engine/configs/assetsConfig.h"
#include "models/editorModel.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    class AssetsWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit AssetsWindow(const std::string &id);

        explicit AssetsWindow(const std::string &id, UiElement *parentElement);

        ~AssetsWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        [[nodiscard]] const char *getTitle() override { return _title; }

    protected:
        void awake() override;

        void subscribe() override;

        void unsubscribe() override;

    private:
        bool _isUpdateAfterMove = true;
        const char *_title = Id.c_str();
        AssetsConfig &_assetConfig;
        std::vector<FolderUiElement *> _folderUiElements;
        std::vector<FileUiElement *> _fileUiElements;
        FolderUiElement *_draggedFolderUiElementCopy = nullptr;
        FileUiElement *_draggedFileUiElementCopy = nullptr;
        EditorModel *_editorModel = nullptr;

        [[nodiscard]] FolderUiElement *getFolderUiElementByEngineFolder(const Folder *folder) const;

        [[nodiscard]] FileUiElement *getFileUiElementByPath(const File *file) const;

        FolderUiElement &createFolderUiElement(const Folder *folder);

        FileUiElement &createFileUiElement(const File *file);

        void recalculateUiFolders(Folder *folder, int &nodeOrder, bool isParentExpanded = true);

        void recalculateUiFiles(const File *file, int &nodeOrder, int depth, bool isParentExpanded);

        void onFileSelected(const FileUiElement *fileUiElement);

        void onElementDragStarted(UiElement *uiElement);

        void onFolderElementDragEnded(UiElement *uiElement);

        void onFileElementDragEnded(UiElement *uiElement);

        void renameAsset(const std::string &guid);

        void deleteAsset(const std::string &guid);

        void rebuild();
    };
} // BreadEditor
