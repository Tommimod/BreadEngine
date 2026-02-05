#include "mandatoryProjectFilesValidator.h"
#include <fstream>
#include "editor.h"
#include "models/reservedFileNames.h"

namespace BreadEditor {
    bool MandatoryProjectFilesValidator::validate()
    {
        try
        {
            std::string path = Editor::getInstance().getEditorModel().getProjectAssetsPath();
            if (path.empty())
            {
                return false;
            }

            auto filePath = TextFormat("%s%s", path.c_str(), ReservedFileNames::PROJECT_SETTINGS_NAME);
            if (!FileExists(filePath))
            {
                std::ofstream outfile(filePath);
                outfile.close();
            }

            filePath = TextFormat("%s%s", path.c_str(), ReservedFileNames::EDITOR_IN_PROJECT_SETTINGS_NAME);
            if (!FileExists(filePath))
            {
                std::ofstream outfile(filePath);
                outfile.close();
            }

            return true;
        }
        catch (std::exception &ex)
        {
            TraceLog(LOG_ERROR, ex.what());
            return false;
        }
    }
} // BreadEditor
