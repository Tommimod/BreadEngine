#include "editorPrefsConfig.h"
#include <fstream>
#include "editor.h"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include "models/reservedFileNames.h"

namespace BreadEditor {
    EditorPrefsConfig::EditorPrefsConfig() : BaseYamlConfig()
    {
    }

    EditorPrefsConfig::EditorPrefsConfig(const char *filePath) : BaseYamlConfig(filePath)
    {
    }

    void EditorPrefsConfig::serializeConfig()
    {
        std::ofstream process(_filePath);
        process.clear();
        process << YAML::Node(*this);
        process.close();
    }

    void EditorPrefsConfig::deserializeConfig()
    {
        auto &configProvider = Editor::getInstance().getConfigsProvider();
        if (configProvider.getEditorPrefsConfig() != nullptr)
        {
            return;
        }

        const auto rawConfig = YAML::LoadFile(_filePath);
        if (rawConfig.IsNull())
        {
            LastProjectPath = TextFormat("%s\\%s", GetWorkingDirectory(), "assets\\game");
            LastOpenedNodePath = LastProjectPath + "\\" + "Root" + ReservedFileNames::MARKER_NODE;
            serializeConfig();
            deserializeConfig();
            return;
        }

        auto data = rawConfig.as<EditorPrefsConfig>();
        data.setPath(_filePath);
        configProvider.setEditorPrefsConfig(std::make_unique<EditorPrefsConfig>(std::move(data)));
    }
} // BreadEditor
