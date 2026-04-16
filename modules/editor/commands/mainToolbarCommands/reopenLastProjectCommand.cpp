#include "reopenLastProjectCommand.h"
#include "editor.h"
#include "validators/mandatoryProjectFilesValidator.h"

namespace BreadEditor {
    void ReopenLastProjectCommand::execute()
    {
        const auto path = Editor::getInstance().getConfigsProvider().getEditorPrefsConfig()->LastProjectPath;
        Editor::getInstance().getEditorModel().setProjectPath(path);
        const auto title = TextFormat("Bread Engine - Editor: %s", path.c_str());
        SetWindowTitle(title);

        if (!MandatoryProjectFilesValidator::validate())
        {
            throw std::runtime_error("Project is not valid");
        }
    }

    void ReopenLastProjectCommand::undo()
    {
    }
} // BreadEditor
