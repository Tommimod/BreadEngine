#include "saveProjectCommand.h"
#include "editor.h"

namespace BreadEditor {
    void SaveProjectCommand::execute()
    {
        Editor::getInstance().mainWindow.getNodeTree().save();
        Engine::getInstance().getAssetsConfig().serializeConfig();
        Engine::getInstance().getGlobalLightSettings().serializeConfig();
    }

    void SaveProjectCommand::undo()
    {
    }
} // BreadEditor
