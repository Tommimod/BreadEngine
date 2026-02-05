#include "openProjectCommand.h"
#include "editor.h"
#include "utils/fileDialogHelper.h"
#include "validators/mandatoryProjectFilesValidator.h"

namespace BreadEditor {
    void OpenProjectCommand::execute()
    {
        auto path = FileDialogHelper::SelectFolderUTF8();
        Editor::getInstance().getEditorModel().setProjectPath(std::move(path));
        if (!MandatoryProjectFilesValidator::validate())
        {
            throw std::runtime_error("Project is not valid");
        }

        TraceLog(LOG_INFO, TextFormat("Project opened: %s", Editor::getInstance().getEditorModel().getProjectPath().c_str()));
    }

    void OpenProjectCommand::undo()
    {
    }
} // BreadEditor
