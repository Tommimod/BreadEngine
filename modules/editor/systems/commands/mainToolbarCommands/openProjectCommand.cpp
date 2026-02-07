#include "openProjectCommand.h"
#include "editor.h"
#include "utils/fileDialogHelper.h"
#include "validators/mandatoryProjectFilesValidator.h"

namespace BreadEditor {
    void OpenProjectCommand::execute()
    {
        auto path = FileDialogHelper::SelectFolderUTF8();
        Editor::getInstance().getEditorModel().setProjectPath(path);
        if (!MandatoryProjectFilesValidator::validate())
        {
            throw std::runtime_error("Project is not valid");
        }

        const auto title = TextFormat("Bread Engine - Editor: %s", path.c_str());
        SetWindowTitle(title);
        Editor::getInstance().getConfigsProvider().getEditorPrefsConfig()->LastProjectPath = std::move(path);
        TraceLog(LOG_INFO, TextFormat("Project opened: %s", Editor::getInstance().getEditorModel().getProjectPath().c_str()));
    }

    void OpenProjectCommand::undo()
    {
    }
} // BreadEditor
