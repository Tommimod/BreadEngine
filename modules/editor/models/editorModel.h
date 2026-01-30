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

    private:
        NodeUiElement *_selectedNodeUiElement = nullptr;
        std::unique_ptr<WindowsModel> _windowsModel;
    };
} // BreadEditor
