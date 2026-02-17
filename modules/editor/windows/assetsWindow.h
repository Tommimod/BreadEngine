#pragma once
#include "editorElements/fileUiElement.h"
#include "editorElements/folderUiElement.h"
#include "../../engine/configs/assetsConfig.h"
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
        const char *_title = Id.c_str();
        AssetsConfig &_fileSystem;
        std::vector<FolderUiElement *> _folderUiElements;
        std::vector<FileUiElement *> _fileUiElements;

        [[nodiscard]] FolderUiElement *getFolderUiElementByEngineFolder(const Folder *folder) const;

        [[nodiscard]] FileUiElement *getFileUiElementByPath(const File *file) const;

        FolderUiElement &CreateFolderUiElement(Folder *folder);

        FileUiElement &CreateFileUiElement(File *file);

        void recalculateUiFolders(Folder *folder, int &nodeOrder, bool isParentExpanded = true);

        void recalculateUiFiles(File *file, int &nodeOrder, int depth, bool isParentExpanded);

        void onFileSelected(const FileUiElement *fileUiElement);
    };
} // BreadEditor
