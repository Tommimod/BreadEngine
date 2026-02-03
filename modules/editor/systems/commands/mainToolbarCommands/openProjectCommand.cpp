#include "openProjectCommand.h"

#include "editor.h"
#include "utils/fileDialogHelper.h"

namespace BreadEditor {
    void OpenProjectCommand::execute()
    {
        Editor::getInstance().getEditorModel().setProjectPath(FileDialogHelper::SelectFolderUTF8());
    }

    void OpenProjectCommand::undo()
    {
    }
} // BreadEditor
