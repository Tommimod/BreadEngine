#include "openProjectCommand.h"
#include "editor.h"
#include "utils/fileDialogHelper.h"
#include "validators/mandatoryProjectFilesValidator.h"

namespace BreadEditor {
    void OpenProjectCommand::execute()
    {
        const auto path = FileDialogHelper::SelectFolderUTF8();
        Editor::getInstance().getEditorModel().setProjectPath(path);
        if (!MandatoryProjectFilesValidator::validate())
        {
            throw std::runtime_error("Project is not valid");
        }

        const auto title = TextFormat("Bread Engine - Editor: %s", path.c_str());
        SetWindowTitle(title);
        const auto config = Editor::getInstance().getConfigsProvider().getEditorPrefsConfig();
        config->LastProjectPath = path;
        config->serialize();
    }

    void OpenProjectCommand::undo()
    {
    }
} // BreadEditor
