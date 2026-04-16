#include "changeParentNodeCommand.h"

namespace BreadEditor {
    ChangeParentNodeCommand::ChangeParentNodeCommand(BreadEngine::Node *node, BreadEngine::Node *nextParent)
    {
        _node = node;
        _nextParent = nextParent;
        _prevParent = _node->getParent();
    }

    void ChangeParentNodeCommand::execute()
    {
        _node->changeParent(_nextParent);
    }

    void ChangeParentNodeCommand::undo()
    {
        _node->changeParent(_prevParent);
    }
} // BreadEditor
