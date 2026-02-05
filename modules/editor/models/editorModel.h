#pragma once
#include "windowsModel.h"
#include "../windows/nodeTreeWindow.h"

namespace BreadEditor {
    class EditorModel
    {
    public:
        Action<NodeUiElement *> onNodeSelected;

        EditorModel()
        {
            _windowsModel = std::make_unique<WindowsModel>();
        }

        ~EditorModel()
        {
            onNodeSelected.unsubscribeAll();
        };

        [[nodiscard]] NodeUiElement *getSelectedNodeUiElement() const { return _selectedNodeUiElement; }

        void setSelectedNodeUiElement(NodeUiElement *nodeUiElement)
        {
            _selectedNodeUiElement = nodeUiElement;
            onNodeSelected.invoke(_selectedNodeUiElement);
        }

        [[nodiscard]] WindowsModel *getWindowsModel() const { return _windowsModel.get(); }

        [[nodiscard]] std::string &getProjectPath() { return _projectPath; }

        void setProjectPath(std::string path)
        {
            _projectPath = std::move(path);
        }

        [[nodiscard]] const char *getProjectAssetsPath() const { return TextFormat("%s\\assets\\", _projectPath.c_str()); }

        [[nodiscard]] static const char *getEditorAssetsPath()
        {
            constexpr auto path = R"(\assets\editor\)";
            return path;
        }

    private:
        NodeUiElement *_selectedNodeUiElement = nullptr;
        std::unique_ptr<WindowsModel> _windowsModel;
        std::string _projectPath;
    };
} // BreadEditor
