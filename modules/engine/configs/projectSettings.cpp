#include "projectSettings.h"

#include <fstream>

#include "engine.h"
#include "models/reservedFileNames.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(ProjectSettings)

    void ProjectSettings::serializeConfig()
    {
        std::ofstream process(_filePath);
        process.clear();
        process << serialize();
        process.close();
    }

    void ProjectSettings::deserializeConfig(const char *filePath)
    {
        _filePath = filePath;
        const auto rawConfig = YAML::LoadFile(_filePath);
        if (rawConfig.IsNull())
        {
            auto &node = NodeProvider::createNode();
            node.setName("Root");
            Engine::setNodeAsRoot(node);
            const auto path = TextFormat("%s%s%s", Engine::getProjectPath().c_str(), node.getName().c_str(), ReservedFileNames::MARKER_NODE);
            std::ofstream process(path);
            process << "";
            process.close();
            node.serialize(path);
            serializeConfig();
            return;
        }

        deserialize(rawConfig);
    }
} // BreadEngine
