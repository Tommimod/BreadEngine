#include "changeNameNodeCommand.h"

#include "editor.h"
#include "engine.h"
#include "commands/commandsHandler.h"
#include "commands/mainToolbarCommands/saveProjectCommand.h"

namespace BreadEditor {
    ChangeNameNodeCommand::ChangeNameNodeCommand(Node *node, const std::string &newName, const std::string &oldName)
    {
        _node = node;
        _newName = newName;
        _oldName = oldName;
    }

    void ChangeNameNodeCommand::execute()
    {
        _node->setName(_newName);
        if (_node->getId() != _node->getRoot().getId()) return;

        auto &assetsConfig = Engine::getInstance().getAssetsConfig();
        const auto &projectSettings = Engine::getInstance().getProjectSettings();
        _renamedFileGuid = projectSettings.startNodeGuid;
        assetsConfig.renameFile(_renamedFileGuid, _newName + BreadEngine::ReservedFileNames::MARKER_NODE);
        Editor::getInstance().mainWindow.getAssetsWindow().rebuild();
        CommandsHandler::execute(std::make_unique<SaveProjectCommand>());
    }

    void ChangeNameNodeCommand::undo()
    {
        _node->setName(_oldName);
        Editor::getInstance().getEditorModel().invokeRefreshInspectorRequested();
        if (_renamedFileGuid.empty()) return;
        auto &assetsConfig = Engine::getInstance().getAssetsConfig();
        if (assetsConfig.getFileByGuid(_renamedFileGuid) == nullptr) return;
        assetsConfig.renameFile(_renamedFileGuid, _oldName + BreadEngine::ReservedFileNames::MARKER_NODE);
        assetsConfig.ConfigUndo.invoke();
        CommandsHandler::execute(std::make_unique<SaveProjectCommand>());
    }
} // BreadEditor
