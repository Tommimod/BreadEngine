#pragma once
#include "windowsModel.h"
#include "editorElements/fileUiElement.h"
#include "editorElements/nodeUiElement.h"

namespace BreadEditor {
    class EditorModel
    {
    public:
        Action<> onClearSelection;
        Action<NodeUiElement *> onNodeSelected;
        Action<FileUiElement *> onFileSelected;

        EditorModel()
        {
            _windowsModel = std::make_unique<WindowsModel>();
        }

        ~EditorModel()
        {
            onNodeSelected.unsubscribeAll();
            onFileSelected.unsubscribeAll();
            onClearSelection.unsubscribeAll();
        };

        [[nodiscard]] NodeUiElement *getSelectedNodeUiElement() const { return _selectedNodeUiElement; }
        [[nodiscard]] FileUiElement *getSelectedFileUiElement() const { return _selectedFileUiElement; }

        void selectNodeUiElement(NodeUiElement *nodeUiElement);

        void selectFileUiElement(FileUiElement *fileUiElement);

        [[nodiscard]] WindowsModel &getWindowsModel() const { return *_windowsModel; }

        [[nodiscard]] std::string &getProjectPath() { return _projectPath; }

        void setProjectPath(std::string path);

        [[nodiscard]] static const char *getEditorAssetsPath();

    private:
        NodeUiElement *_selectedNodeUiElement = nullptr;
        FileUiElement *_selectedFileUiElement = nullptr;
        std::unique_ptr<WindowsModel> _windowsModel;
        std::string _projectPath;

        void clearSelections();
    };
} // BreadEditor
