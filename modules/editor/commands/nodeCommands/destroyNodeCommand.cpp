#include "destroyNodeCommand.h"

namespace BreadEditor {
    DestroyNodeCommand::DestroyNodeCommand(BreadEngine::Node *node)
    {
        _node = node;
        _data = BreadEngine::Node::getDataForCopy(*node);
        _parentId = node->getParent()->getId();
    }

    void DestroyNodeCommand::execute()
    {
        if (_node == nullptr) return;
        _node->destroy();
        _node = nullptr;
    }

    void DestroyNodeCommand::undo()
    {
        const auto node = BreadEngine::NodeProvider::getNode(_parentId);
        if (node == nullptr) return;
        BreadEngine::Node::createCopyFromData(_data, *node);
    }
} // BreadEditor
