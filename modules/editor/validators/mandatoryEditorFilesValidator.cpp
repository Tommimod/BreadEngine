#include "mandatoryEditorFilesValidator.h"
#include <fstream>
#include "editor.h"
#include "../models/reservedFileNames.h"

namespace BreadEditor {
    bool MandatoryEditorFilesValidator::validate()
    {
        try
        {
            const auto filePath = TextFormat("%s%s%s", GetApplicationDirectory(), EditorModel::getEditorAssetsPath(), ReservedFileNames::EDITOR_PREFS_NAME);
            if (!FileExists(filePath))
            {
                std::ofstream outfile(filePath);
                outfile.close();
            }

            EditorPrefsConfig(filePath).deserialize();
            return true;
        }
        catch (std::exception &ex)
        {
            TraceLog(LOG_ERROR, ex.what());
            return false;
        }
    }
} // BreadEditor
