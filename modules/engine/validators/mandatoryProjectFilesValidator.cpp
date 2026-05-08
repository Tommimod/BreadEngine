#include "mandatoryProjectFilesValidator.h"

#include <fstream>

#include "engine.h"
#include "raylib.h"
#include "models/reservedFileNames.h"

namespace BreadEngine {
    bool MandatoryProjectFilesValidator::validateAndInitialize()
    {
        try
        {
            std::string path = Engine::getProjectPath();
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
            Engine::getInstance().getProjectSettings().deserializeConfig(filePath);

            filePath = TextFormat("%s%s", path.c_str(), ReservedFileNames::EDITOR_IN_PROJECT_SETTINGS_NAME);
            if (!FileExists(filePath))
            {
                std::ofstream outfile(filePath);
                outfile.close();
            }

            filePath = TextFormat("%s%s", path.c_str(), ReservedFileNames::ASSETS_REGISTRY_NAME);
            if (!FileExists(filePath))
            {
                std::ofstream outfile(filePath);
                outfile.close();
            }
            Engine::getInstance().getAssetsConfig().deserializeConfig(filePath);

            return true;
        }
        catch (std::exception &ex)
        {
            TraceLog(LOG_ERROR, ex.what());
            return false;
        }
    }
} // BreadEditor
