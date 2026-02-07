#pragma once
#include <string>

#include "nameof.h"
#include "infrastructure/baseYamlConfig.h"

namespace BreadEditor {
    struct EditorPrefsConfig : BaseYamlConfig
    {
    public:
        std::string LastProjectPath;

        EditorPrefsConfig();

        bool operator==(const EditorPrefsConfig &d) const
        {
            return LastProjectPath == d.LastProjectPath;
        }

        EditorPrefsConfig(const char *filePath);

        ~EditorPrefsConfig() override = default;

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
            node.push_back(rhs.LastProjectPath);
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
