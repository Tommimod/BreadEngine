#include "createNewProjectCommand.h"
#include "utils/fileDialogHelper.h"

namespace BreadEditor {
    CreateNewProjectCommand::CreateNewProjectCommand() = default;
    CreateNewProjectCommand::~CreateNewProjectCommand()= default;

    void CreateNewProjectCommand::execute()
    {
        auto pathForProject = FileDialogHelper::SelectFolderUTF8();
    }

    void CreateNewProjectCommand::undo()
    {
    }
} // BreadEditor
