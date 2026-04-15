#pragma once
#include "windowsModel.h"
#include "editorElements/fileUiElement.h"
#include "editorElements/nodeUiElement.h"

namespace BreadEditor {
    class EditorModel
    {
    public:
        Action<UiElement *> onDragEnded;
        Action<> onClearSelection;
        Action<Node *> onNodeSelected;
        Action<FileUiElement *> onFileSelected;
        Action<Node *> onNodeHighlightRequested;

        EditorModel()
        {
            _windowsModel = std::make_unique<WindowsModel>();
        }

        ~EditorModel()
        {
            onNodeSelected.unsubscribeAll();
            onFileSelected.unsubscribeAll();
            onClearSelection.unsubscribeAll();
            onDragEnded.unsubscribeAll();
            onNodeHighlightRequested.unsubscribeAll();
        };

        [[nodiscard]] Node *getSelectedEngineNode() const { return _selectedNodeUiElement; }
        [[nodiscard]] FileUiElement *getSelectedFileUiElement() const { return _selectedFileUiElement; }
        [[nodiscard]] UiElement *getDraggableElement() const { return _draggableElement; }

        void invokeNodeHighlightRequested(Node *node);

        void setDraggableElement(UiElement *draggableElement);

        void selectNodeUiElement(const NodeUiElement *nodeUiElement);

        void selectFileUiElement(FileUiElement *fileUiElement);

        [[nodiscard]] WindowsModel &getWindowsModel() const { return *_windowsModel; }

        [[nodiscard]] std::string &getProjectPath() { return _projectPath; }

        void setProjectPath(std::string path);

        [[nodiscard]] static const char *getEditorAssetsPath();

    private:
        Node *_selectedNodeUiElement = nullptr;
        FileUiElement *_selectedFileUiElement = nullptr;
        UiElement *_draggableElement = nullptr;
        std::unique_ptr<WindowsModel> _windowsModel;
        std::string _projectPath;

        void clearSelections();
    };
} // BreadEditor
