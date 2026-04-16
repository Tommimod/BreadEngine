#include "addComponentCommand.h"

namespace BreadEditor {
    AddComponentCommand::AddComponentCommand(BreadEngine::Node *node, const std::string &componentType)
    {
        _node = node;
        _componentType = componentType;
    }

    void AddComponentCommand::execute()
    {
        const auto id = _node->getId();
        BreadEngine::ComponentsProvider::addDynamic(id, _componentType);
    }

    void AddComponentCommand::undo()
    {
        BreadEngine::ComponentsProvider::remove(_node->getId(), _componentType);
    }
} // BreadEditor
