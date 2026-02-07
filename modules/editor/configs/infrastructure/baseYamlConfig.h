#pragma once

namespace BreadEditor {
    struct BaseYamlConfig
    {
        explicit BaseYamlConfig(const char *filePath);

        virtual ~BaseYamlConfig();

        virtual void serialize() = 0;

        virtual void deserialize() = 0;

    protected:
        void setPath(const char *filePath) { _filePath = filePath; }
        const char *_filePath = nullptr;
    };
} // BreadEditor
