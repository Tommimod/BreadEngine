#include "saveProjectCommand.h"
#include "editor.h"

namespace BreadEditor {
    void SaveProjectCommand::execute()
    {
        Editor::getInstance().mainWindow.getNodeTree().save();
    }

    void SaveProjectCommand::undo()
    {
    }
} // BreadEditor
