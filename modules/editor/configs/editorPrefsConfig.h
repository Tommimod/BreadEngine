#pragma once
#include <string>

#include "nameof.h"
#include "../../engine/configs/baseYamlConfig.h"

namespace BreadEditor {
    struct EditorPrefsConfig : BreadEngine::BaseYamlConfig
    {
        std::string LastProjectPath;
        std::string LastOpenedNodePath;

        EditorPrefsConfig();

        explicit EditorPrefsConfig(const char *filePath);

        ~EditorPrefsConfig() override = default;

        bool operator==(const EditorPrefsConfig &d) const
        {
            return LastProjectPath == d.LastProjectPath;
        }

        void serializeConfig() override;

        void deserializeConfig() override;
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
            const auto projectPathName = NAMEOF(rhs.LastProjectPath).c_str();
            const auto openedNodeName = NAMEOF(rhs.LastOpenedNodePath).c_str();
            node[projectPathName] = rhs.LastProjectPath;
            node[openedNodeName] = rhs.LastOpenedNodePath;
            return node;
        }

        static bool decode(const Node &node, BreadEditor::EditorPrefsConfig &rhs)
        {
            const auto projectPathName = NAMEOF(rhs.LastProjectPath).c_str();
            const auto openedNodeName = NAMEOF(rhs.LastOpenedNodePath).c_str();
            rhs.LastProjectPath = node[projectPathName].as<std::string>();
            rhs.LastOpenedNodePath = node[openedNodeName].as<std::string>();
            return true;
        }
    };
}
