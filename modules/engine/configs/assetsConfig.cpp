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

    std::string AssetsConfig::getNewGUID()
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd() ^ std::chrono::steady_clock::now().time_since_epoch().count());

        std::uniform_int_distribution<uint64_t> dist(0, static_cast<uint64_t>(-1));

        uint64_t part1 = dist(gen);
        uint64_t part2 = dist(gen);

        part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        ss << std::setw(8) << (part1 >> 32)
                << std::setw(4) << ((part1 >> 16) & 0xFFFF)
                << std::setw(4) << (part1 & 0xFFFF)
                << std::setw(4) << ((part2 >> 48) & 0xFFFF)
                << std::setw(12) << (part2 & 0xFFFFFFFFFFFFULL);

        return ss.str();
    }

    void AssetsConfig::parseFolders(Folder &folder, const FilePathList &filePathList) noexcept
    {
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = AssetsConfig::isFolder(path);
            const auto relativePathStart = path + strlen(_projectPath.c_str()) + 1;
            const auto fileName = strrchr(relativePathStart, '\\');
            auto pathFromRoot = fileName ? fileName + 1 : relativePathStart;
            if (isFolder)
            {
                auto subFolder = Folder(folder.getDepth() + 1, pathFromRoot, getNewGUID());
                auto list = LoadDirectoryFiles(path);
                parseFolders(subFolder, list);
                folder.getFolders().push_back(std::move(subFolder));
                UnloadDirectoryFiles(list);
            }
            else
            {
                const auto ext = GetFileExtension(path);
                auto file = File(path, pathFromRoot, ext, getNewGUID());
                folder.getFiles().push_back(std::move(file));
            }
        }
    }
} // BreadEngine
