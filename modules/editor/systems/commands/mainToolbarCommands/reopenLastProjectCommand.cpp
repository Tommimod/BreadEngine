#include "reopenLastProjectCommand.h"

#include "editor.h"

namespace BreadEditor {
    void ReopenLastProjectCommand::execute()
    {
        const auto path = Editor::getInstance().getConfigsProvider().getEditorPrefsConfig()->LastProjectPath;
        if (!path.empty())
        {
            const auto title = TextFormat("Bread Engine - Editor: %s", path.c_str());
            SetWindowTitle(title);
        }
    }

    void ReopenLastProjectCommand::undo()
    {
    }
} // BreadEditor