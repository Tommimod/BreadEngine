#include "openProjectCommand.h"

#include "editor.h"
#include "utils/fileDialogHelper.h"
#include "validators/mandatoryFilesValidator.h"

namespace BreadEditor {
    void OpenProjectCommand::execute()
    {
        auto path = FileDialogHelper::SelectFolderUTF8();
        Editor::getInstance().getEditorModel().setProjectPath(std::move(path));
        MandatoryFilesValidator validator;
        const auto isValid = validator.validate();
        if (!isValid)
        {
            throw std::runtime_error("Project is not valid");
        }
    }

    void OpenProjectCommand::undo()
    {
    }
} // BreadEditor
