#include "mandatoryFilesValidator.h"

#include <fstream>
#include "editor.h"
#include "models/reservedFileNames.h"

namespace BreadEditor {
    MandatoryFilesValidator::MandatoryFilesValidator()
    = default;

    MandatoryFilesValidator::~MandatoryFilesValidator()
    = default;

    bool MandatoryFilesValidator::validate()
    {
        try
        {
            std::string path = Editor::getInstance().getEditorModel().getProjectAssetsPath();
            if (path.empty())
            {
                return false;
            }

            if (!FileExists(TextFormat("%s%s", path.c_str(), ReservedFileNames::PROJECT_SETTINGS_NAME)))
            {
                std::ofstream outfile(ReservedFileNames::PROJECT_SETTINGS_NAME);
                outfile.close();
            }

            if (!FileExists(TextFormat("%s%s", path.c_str(), ReservedFileNames::EDITOR_SETTINGS_NAME)))
            {
                std::ofstream outfile(ReservedFileNames::EDITOR_SETTINGS_NAME);
                outfile.close();
            }

            return true;
        }
        catch (std::exception &ex)
        {
            return false;
        }
    }
} // BreadEditor
