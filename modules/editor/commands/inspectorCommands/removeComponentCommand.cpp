#include "removeComponentCommand.h"

#include "editor.h"

namespace BreadEditor {
    RemoveComponentCommand::RemoveComponentCommand(Node *node, const std::type_index type) : _type(type)
    {
        _node = node;
    }

    void RemoveComponentCommand::execute()
    {
        _originalComponent = ComponentsProvider::removeAndGetOwnership(_node->getId(), _type);
    }

    void RemoveComponentCommand::undo()
    {
        ComponentsProvider::addDynamic(_node->getId(), _type, std::move(_originalComponent));
        Editor::getInstance().getEditorModel().invokeRefreshInspectorRequested();
    }
} // BreadEditor
