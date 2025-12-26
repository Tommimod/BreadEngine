#pragma once
#include "../windows/mainWindow/nodeTreeWindow.h"

namespace BreadEditor {
    class EditorModel
    {
    public:
        Action<NodeUiElement *> onNodeSelected;

        EditorModel() = default;

        ~EditorModel()
        {
            onNodeSelected.unsubscribeAll();
        };

        NodeUiElement *getSelectedNodeUiElement() const { return _selectedNodeUiElement; }
        void setSelectedNodeUiElement(NodeUiElement *nodeUiElement)
        {
            _selectedNodeUiElement = nodeUiElement;
            onNodeSelected.invoke(_selectedNodeUiElement);
        }

    private:
        NodeUiElement *_selectedNodeUiElement = nullptr;
    };
} // BreadEditor
