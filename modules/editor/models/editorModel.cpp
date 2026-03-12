#include "editorModel.h"

namespace BreadEditor {
    void EditorModel::selectNodeUiElement(NodeUiElement *nodeUiElement)
    {
        clearSelections();
        if (nodeUiElement == nullptr) return;

        _selectedNodeUiElement = nodeUiElement;
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
