#pragma once
#include <string>

namespace BreadEngine {
    struct BaseYamlConfig
    {
        explicit BaseYamlConfig() = default;
        explicit BaseYamlConfig(const std::string &filePath);

        virtual ~BaseYamlConfig();

        virtual void serialize() = 0;

        virtual void deserialize() = 0;

    protected:
        void setPath(const std::string &filePath) { _filePath = filePath; }
        std::string _filePath;
    };
} // BreadEngine
