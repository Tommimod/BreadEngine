#include "baseYamlConfig.h"

namespace BreadEngine {
    BaseYamlConfig::BaseYamlConfig(const std::string &filePath)
    {
        _filePath = filePath;
    }

    BaseYamlConfig::~BaseYamlConfig()
    = default;
} // BreadEngine
