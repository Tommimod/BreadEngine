#pragma once
#include "file.h"
#include "inspectorObject.h"

namespace BreadEngine {
    struct Asset : InspectorStruct
    {
        Asset() = default;

        explicit Asset(const std::string &fileGuid)
        {
            _fileGuid = fileGuid;
        }

        ~Asset() override = default;

        virtual bool operator==(const Asset &other) const
        {
            return getFile() == other.getFile();
        }

        bool operator!=(const Asset &other) const
        {
            return !(*this == other);
        }

        [[nodiscard]] const std::string &getGuid() const { return _fileGuid; }

        [[nodiscard]] const std::string &getAssetName() const { return getFile()->getShortName(); }

        [[nodiscard]] const std::string &getAssetPath() const { return getFile()->getFullPath(); }

        virtual void loadToMemory() = 0;

    protected:
        std::string _fileGuid;

        File *getFile() const;

        INSPECTOR_BEGIN(Asset)
        INSPECTOR_END()
    };
} // BreadEngine
