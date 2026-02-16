#pragma once
#include <string>

#include "nameof.h"
#include "../../engine/configs/baseYamlConfig.h"

namespace BreadEditor {
    struct EditorPrefsConfig : BreadEngine::BaseYamlConfig
    {
        std::string LastProjectPath;

        EditorPrefsConfig();

        explicit EditorPrefsConfig(const char *filePath);

        ~EditorPrefsConfig() override = default;

        bool operator==(const EditorPrefsConfig &d) const
        {
            return LastProjectPath == d.LastProjectPath;
        }

        void serialize() override;

        void deserialize() override;
    };
} // BreadEditor

#include <yaml-cpp/node/node.h>

namespace YAML {
    template<>
    struct convert<BreadEditor::EditorPrefsConfig>
    {
        static Node encode(const BreadEditor::EditorPrefsConfig &rhs)
        {
            Node node;
            const auto name = NAMEOF(rhs.LastProjectPath).c_str();
            node[name] = rhs.LastProjectPath;
            return node;
        }

        static bool decode(const Node &node, BreadEditor::EditorPrefsConfig &rhs)
        {
            const auto name = NAMEOF(rhs.LastProjectPath).c_str();
            rhs.LastProjectPath = node[name].as<std::string>();
            return true;
        }
    };
}
