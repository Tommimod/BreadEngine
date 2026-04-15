#include "editorModel.h"

namespace BreadEditor {
    void EditorModel::invokeNodeHighlightRequested(Node *node)
    {
        onNodeHighlightRequested.invoke(node);
    }

    void EditorModel::setDraggableElement(UiElement *draggableElement)
    {
        if (_draggableElement != nullptr && draggableElement == nullptr)
        {
            onDragEnded.invoke(_draggableElement);
        }

        _draggableElement = draggableElement;
    }

    void EditorModel::selectNodeUiElement(const NodeUiElement *nodeUiElement)
    {
        clearSelections();
        if (nodeUiElement == nullptr) return;

        _selectedNodeUiElement = nodeUiElement->getNode();
        onNodeSelected.invoke(_selectedNodeUiElement);
    }

    void EditorModel::selectFileUiElement(FileUiElement *fileUiElement)
    {
        clearSelections();
        if (fileUiElement == nullptr) return;

        _selectedFileUiElement = fileUiElement;
        onFileSelected.invoke(_selectedFileUiElement);
    }

    void EditorModel::setProjectPath(std::string path)
    {
        _projectPath = std::move(path) + "\\";
    }

    const char *EditorModel::getEditorAssetsPath()
    {
        constexpr auto path = R"(\assets\editor\)";
        return path;
    }

    void EditorModel::clearSelections()
    {
        _selectedNodeUiElement = nullptr;
        _selectedFileUiElement = nullptr;
        onClearSelection.invoke();
    }
} // BreadEditor
