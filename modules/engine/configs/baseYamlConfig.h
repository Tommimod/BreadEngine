#pragma once
#include <string>

#include "inspectorObject.h"

namespace BreadEngine {
    struct BaseYamlConfig : InspectorStruct
    {
        explicit BaseYamlConfig() = default;

        explicit BaseYamlConfig(const std::string &filePath);

        ~BaseYamlConfig() override;

        virtual void serializeConfig() = 0;

        virtual void deserializeConfig(const char* filePath) = 0;

    protected:
        std::string _filePath;
    };
} // BreadEngine
