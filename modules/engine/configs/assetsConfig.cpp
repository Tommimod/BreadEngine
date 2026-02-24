#include "assetsConfig.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include "raylib.h"
#include "models/reservedFileNames.h"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include "assetsConfigYaml.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(AssetsConfig)
    DEFINE_STATIC_PROPS(Folder)
    DEFINE_STATIC_PROPS(File)

    void AssetsConfig::serialize()
    {
        const auto filePath = std::string(_projectPath) + "\\" + ReservedFileNames::ASSETS_REGISTRY_NAME;
        std::ofstream process(filePath);
        process.clear();
        process << YAML::Node(*this);
        process.close();
    }

    void AssetsConfig::deserialize(const char *projectPath)
    {
        _projectPath = projectPath;
        auto filePath = std::string(projectPath) + "\\" + ReservedFileNames::ASSETS_REGISTRY_NAME;
        auto rawConfig = YAML::LoadFile(filePath);
        if (rawConfig.IsNull())
        {
            findAllAssets(projectPath);
            serialize();
            return;
        }

        auto data = rawConfig.as<AssetsConfig>();
        _projectPath = data._projectPath;
        _rootFolder = data._rootFolder;
    }

    void AssetsConfig::deserialize()
    {
        deserialize(TextFormat("%s\\%s", GetWorkingDirectory(), "assets\\game"));
    }

    void AssetsConfig::findAllAssets(const char *projectPath)
    {
        _projectPath = projectPath;
        const FilePathList filesProvider = LoadDirectoryFiles(projectPath);
        parseFolders(_rootFolder, filesProvider);
        UnloadDirectoryFiles(filesProvider);
    }

    bool AssetsConfig::isFolder(const char *path)
    {
        const auto paths = LoadDirectoryFiles(path);
        const auto isFolder = paths.count > 0 && GetFileExtension(path) == nullptr;
        UnloadDirectoryFiles(paths);
        return isFolder;
    }

    Folder *AssetsConfig::getRootFolder()
    {
        return &_rootFolder;
    }

    const std::string &AssetsConfig::getPathByGuid(const std::string &guid)
    {
    }

    const std::string &AssetsConfig::getGuidByPath(const std::string &path)
    {
    }

    void AssetsConfig::parseFolders(Folder &folder, const FilePathList &filePathList) noexcept
    {
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = AssetsConfig::isFolder(path);
            if (strncmp(path, _projectPath.c_str(), _projectPath.size()) != 0)
            {
                TraceLog(LOG_ERROR, "Path does not start with project path");
                throw std::runtime_error("Path does not start with project path");
            }

            std::string relPath = path + _projectPath.size() + 1;
            if (isFolder)
            {
                auto subFolder = Folder(folder.getDepth() + 1, relPath, getNewGUID());
                auto list = LoadDirectoryFiles(path);
                parseFolders(subFolder, list);
                folder.getFolders().push_back(std::move(subFolder));
                UnloadDirectoryFiles(list);
            }
            else
            {
                const auto ext = GetFileExtension(path);
                auto file = File(path, relPath, ext, getNewGUID());
                folder.getFiles().push_back(std::move(file));
            }
        }
    }
} // BreadEngine
