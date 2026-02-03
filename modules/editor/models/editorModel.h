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

        void setProjectPath(const std::string &path)
        {
            _projectPath = path;
        }

    private:
        NodeUiElement *_selectedNodeUiElement = nullptr;
        std::unique_ptr<WindowsModel> _windowsModel;
        std::string _projectPath;
    };
} // BreadEditor
