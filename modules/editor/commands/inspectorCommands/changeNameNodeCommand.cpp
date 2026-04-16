#include "changeNameNodeCommand.h"

namespace BreadEditor {
    ChangeNameNodeCommand::ChangeNameNodeCommand(BreadEngine::Node *node, const std::string &newName, const std::string &oldName)
    {
        _node = node;
        _newName = newName;
        _oldName = oldName;
    }

    void ChangeNameNodeCommand::execute()
    {
        _node->setName(_newName);
    }

    void ChangeNameNodeCommand::undo()
    {
        _node->setName(_oldName);
    }
} // BreadEditor
