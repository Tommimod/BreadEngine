#pragma once
#include "inspectorObject.h"

namespace BreadEngine {
    struct Asset : InspectorStruct
    {
        Asset() = default;

        Asset(const std::string &guid, const std::string &name);

        ~Asset() override = default;

        virtual bool operator==(const Asset &other) const
        {
            return _guid == other._guid && _name == other._name;
        }

        bool operator!=(const Asset &other) const
        {
            return !(*this == other);
        }

        [[nodiscard]] const std::string &getGuid() const { return _guid; }

        void setGuid(const std::string &guid) { _guid = guid; }

        [[nodiscard]] const std::string &getName() const { return _name; }

        void setName(const std::string &name) { _name = name; }

    protected:
        std::string _guid;
        std::string _name;

        INSPECTOR_BEGIN(Asset)
            INSPECT_FIELD(_name);
        INSPECTOR_END()
    };
} // BreadEngine