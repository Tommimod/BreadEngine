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
            INSPECT_FIELD(startNodeGuid)
        INSPECTOR_END()
    };
} // BreadEngine

namespace YAML {
    template<>
    struct convert<BreadEngine::ProjectSettings>
    {
        static Node encode(const BreadEngine::ProjectSettings &rhs)
        {
            const auto startNodeGuidName = NAMEOF(rhs.startNodeGuid).c_str();
            Node node;
            node[startNodeGuidName] = rhs.startNodeGuid;
            return node;
        }

        static bool decode(const Node &node, BreadEngine::ProjectSettings &rhs)
        {
            const auto startNodeGuidName = NAMEOF(rhs.startNodeGuid).c_str();
            rhs.startNodeGuid = node[startNodeGuidName].as<std::string>();
            return true;
        }
    };
}
