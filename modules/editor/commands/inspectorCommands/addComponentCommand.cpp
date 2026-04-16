#include "addComponentCommand.h"

#include "editor.h"

namespace BreadEditor {
    AddComponentCommand::AddComponentCommand(Node *node, const std::string &componentType)
    {
        _node = node;
        _componentType = componentType;
    }

    void AddComponentCommand::execute()
    {
        const auto id = _node->getId();
        ComponentsProvider::addDynamic(id, _componentType);
    }

    void AddComponentCommand::undo()
    {
        ComponentsProvider::remove(_node->getId(), _componentType);
        Editor::getInstance().getEditorModel().invokeRefreshInspectorRequested();
    }
} // BreadEditor
