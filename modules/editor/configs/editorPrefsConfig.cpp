#include "editorPrefsConfig.h"
#include <fstream>
#include "editor.h"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>

namespace BreadEditor {
    EditorPrefsConfig::EditorPrefsConfig() : BaseYamlConfig()
    {
    }

    EditorPrefsConfig::EditorPrefsConfig(const char *filePath) : BaseYamlConfig(filePath)
    {
    }

    void EditorPrefsConfig::serialize()
    {
        YAML::Node node;
        const auto name = NAMEOF(LastProjectPath).c_str();
        node[name] = LastProjectPath;

        std::ofstream process(_filePath);
        process.clear();
        process << node;
        process.close();
    }

    void EditorPrefsConfig::deserialize()
    {
        auto &configProvider = Editor::getInstance().getConfigsProvider();
        if (configProvider.getEditorPrefsConfig() != nullptr)
        {
            return;
        }

        auto rawConfig = YAML::LoadFile(_filePath);
        if (rawConfig.IsNull())
        {
            const auto name = NAMEOF(LastProjectPath).c_str();
            rawConfig[name] = TextFormat("%s\\%s", GetWorkingDirectory(), "assets\\game");

            std::ofstream process(_filePath);
            process << rawConfig;
            process.close();
        }

        auto data = rawConfig.as<EditorPrefsConfig>();
        data.setPath(_filePath);
        configProvider.setEditorPrefsConfig(std::make_unique<EditorPrefsConfig>(std::move(data)));
    }
} // BreadEditor
