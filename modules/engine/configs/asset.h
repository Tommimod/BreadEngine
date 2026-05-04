#pragma once
#include "file.h"
#include "inspectorObject.h"

namespace BreadEngine {
    struct Asset : InspectorStruct
    {
        Asset() = default;

        explicit Asset(File *file)
        {
            _file = file;
        }

        ~Asset() override = default;

        virtual bool operator==(const Asset &other) const
        {
            return _file == other._file;
        }

        bool operator!=(const Asset &other) const
        {
            return !(*this == other);
        }

        [[nodiscard]] const std::string &getGuid() const { return _file->getGUID(); }

        [[nodiscard]] const std::string &getAssetName() const { return _file->getShortName(); }

        [[nodiscard]] const std::string &getAssetPath() const { return _file->getFullPath(); }

    protected:
        File *_file = nullptr;

        INSPECTOR_BEGIN(Asset)
        INSPECTOR_END()
    };
} // BreadEngine
