#include "createNodeCommand.h"

namespace BreadEditor {
    CreateNodeCommand::CreateNodeCommand(BreadEngine::Node *parentNode)
    {
        _parentNode = parentNode;
    }

    CreateNodeCommand::CreateNodeCommand(BreadEngine::Node *parentNode, YAML::Node data)
    {
        _parentNode = parentNode;
        _data = std::move(data);
        _withData = true;
    }

    void CreateNodeCommand::execute()
    {
        if (_withData)
        {
            _createdNode = BreadEngine::Node::createCopyFromData(_data, *_parentNode);
        }
        else
        {
            _createdNode = BreadEngine::Node::createCopyFromNode(*_parentNode);
        }
    }

    void CreateNodeCommand::undo()
    {
        _createdNode->destroy();
    }
} // BreadEditor
