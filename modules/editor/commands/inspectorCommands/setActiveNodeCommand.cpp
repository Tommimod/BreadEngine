#include "setActiveNodeCommand.h"

#include "editor.h"

namespace BreadEditor {
    SetActiveNodeCommand::SetActiveNodeCommand(Node *node, const bool nextState)
    {
        _node = node;
        _nextState = nextState;
    }

    void SetActiveNodeCommand::execute()
    {
        _node->setIsActive(_nextState);
    }

    void SetActiveNodeCommand::undo()
    {
        _node->setIsActive(!_node->getIsActive());
        Editor::getInstance().getEditorModel().invokeRefreshInspectorRequested();
    }
} // BreadEditor
