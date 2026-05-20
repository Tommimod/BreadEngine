#include "globalLightSettings.h"

#include <fstream>

namespace BreadEngine {
    DEFINE_STATIC_PROPS(GlobalLightSettings)

    void GlobalLightSettings::serializeConfig()
    {
        std::ofstream process(_filePath);
        process.clear();
        process << serialize();
        process.close();
    }

    void GlobalLightSettings::deserializeConfig(const char *filePath)
    {
        _filePath = filePath;
        const auto rawConfig = YAML::LoadFile(_filePath);
        if (rawConfig.IsNull())
        {
            return;
        }

        deserialize(rawConfig);
    }
} // BreadEngine
