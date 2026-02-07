#include "baseYamlConfig.h"

namespace BreadEditor {
    BaseYamlConfig::BaseYamlConfig(const std::string &filePath)
    {
        _filePath = filePath;
    }

    BaseYamlConfig::~BaseYamlConfig()
    = default;
} // BreadEditor
