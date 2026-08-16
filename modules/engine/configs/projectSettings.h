#pragma once
#include <yaml-cpp/node/node.h>

#include "baseYamlConfig.h"
#include "nameof.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct ProjectSettings : BaseYamlConfig
    {
        std::string startNodeGuid;
        /// Read once at startup, so a change here takes effect on the next launch.
        OutputColorSpace outputColorSpace = OutputColorSpace::Gamma;

        ProjectSettings() = default;

        ~ProjectSettings() override = default;

        void serializeConfig() override;

        void deserializeConfig(const char* filePath) override;

    private:
        INSPECTOR_BEGIN(ProjectSettings)
            INSPECT_FIELD_OPT(startNodeGuid, Property::Options::READONLY)
            INSPECT_FIELD(outputColorSpace)
        INSPECTOR_END()
    };
} // BreadEngine