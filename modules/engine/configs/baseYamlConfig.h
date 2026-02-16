#pragma once
#include <string>

#include "inspectorObject.h"

namespace BreadEngine {
    struct BaseYamlConfig : InspectorStruct
    {
        explicit BaseYamlConfig() = default;

        explicit BaseYamlConfig(const std::string &filePath);

        ~BaseYamlConfig() override;

        virtual void serialize() = 0;

        virtual void deserialize() = 0;

    protected:
        void setPath(const std::string &filePath) { _filePath = filePath; }
        std::string _filePath;
    };
} // BreadEngine
