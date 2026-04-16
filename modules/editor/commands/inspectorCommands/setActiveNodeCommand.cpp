#include "setActiveNodeCommand.h"

namespace BreadEditor {
    SetActiveNodeCommand::SetActiveNodeCommand(BreadEngine::Node *node, const bool nextState)
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
        _node->setIsActive(!_nextState);
    }
} // BreadEditor
