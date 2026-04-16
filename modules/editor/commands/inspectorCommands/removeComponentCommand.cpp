#include "removeComponentCommand.h"

namespace BreadEditor {
    RemoveComponentCommand::RemoveComponentCommand(BreadEngine::Node *node, const std::type_index type) : _type(type)
    {
        _node = node;
    }

    void RemoveComponentCommand::execute()
    {
        auto component = BreadEngine::ComponentsProvider::get(_node->getId(), _type);
        _node->remove(_type);
    }

    void RemoveComponentCommand::undo()
    {
        BreadEngine::ComponentsProvider::addDynamic(_node->getId(), _type, std::move(_originalComponent));
    }
} // BreadEditor