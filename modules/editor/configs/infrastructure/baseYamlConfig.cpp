#include "baseYamlConfig.h"

namespace BreadEditor {
    BaseYamlConfig::BaseYamlConfig(const char *filePath)
    {
        _filePath = filePath;
    }

    BaseYamlConfig::~BaseYamlConfig()
    {
        _filePath = nullptr;
    }
} // BreadEditor
