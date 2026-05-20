#pragma once
#include <yaml-cpp/node/node.h>

#include "baseYamlConfig.h"
#include "nameof.h"

namespace BreadEngine {
    struct ProjectSettings : BaseYamlConfig
    {
        std::string startNodeGuid;

        ProjectSettings() = default;

        ~ProjectSettings() override = default;

        void serializeConfig() override;

        void deserializeConfig(const char* filePath) override;

    private:
        INSPECTOR_BEGIN(ProjectSettings)
            INSPECT_FIELD_OPT(startNodeGuid, Property::Options::READONLY)
        INSPECTOR_END()
    };
} // BreadEngine