#include "createMaterialCommand.h"

#include <fstream>

#include "engine.h"
#include "logger.h"
#include "../../../engine/models/reservedFileNames.h"

namespace BreadEditor {
    constexpr auto materialBaseName = "New Material";

    CreateMaterialCommand::CreateMaterialCommand(const std::string &folderGuid)
        : _assetsConfig(BreadEngine::Engine::getInstance().getAssetsConfig())
    {
        _folderGuid = folderGuid;
    }

    void CreateMaterialCommand::execute()
    {
        const auto folder = _assetsConfig.getFolderByGuid(_folderGuid);
        if (folder == nullptr) return;

        auto path = nextFreePath(folder->getFullPath());
        std::ofstream file(path);
        if (!file)
        {
            BreadEngine::Logger::LogError("Failed to create material " + path);
            return;
        }

        file.close();
        _assetsConfig.onEntityCreated(path);

        const auto assetFile = _assetsConfig.getFileByPath(path);
        if (assetFile == nullptr) return;
        if (const auto asset = _assetsConfig.getAsset(assetFile)) asset->saveToOwnFile();
    }

    void CreateMaterialCommand::undo()
    {
    }

    std::string CreateMaterialCommand::nextFreePath(const std::string &folderPath)
    {
        constexpr auto extension = BreadEngine::ReservedFileNames::MARKER_MATERIAL;
        auto path = folderPath + "\\" + materialBaseName + extension;
        for (auto suffix = 1; FileExists(path.c_str()); ++suffix)
        {
            path = folderPath + "\\" + materialBaseName + " " + std::to_string(suffix) + extension;
        }

        return path;
    }
} // BreadEditor
