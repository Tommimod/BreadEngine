#include "mandatoryEditorFilesValidator.h"
#include <fstream>
#include "editor.h"
#include "models/reservedFileNames.h"

namespace BreadEditor {
    bool MandatoryEditorFilesValidator::validate()
    {
        const auto filePath = TextFormat("%s%s", EditorModel::getEditorAssetsPath(), ReservedFileNames::EDITOR_PREFS_NAME);
        if (!FileExists(filePath))
        {
            std::ofstream outfile(filePath);
            outfile.close();
        }
    }
} // BreadEditor
