#include "assetsConfig.h"
#include <fstream>
#include "raylib.h"
#include "models/reservedFileNames.h"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include "assetsConfigYaml.h"
#include "logger.h"
#include "tracy/Tracy.hpp"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(AssetsConfig)
    DEFINE_STATIC_PROPS(Folder)
    DEFINE_STATIC_PROPS(File)
    namespace fs = std::filesystem;

    void AssetsConfig::serializeConfig()
    {
        ZoneScoped;
        const auto filePath = std::string(_projectPath) + "\\" + ReservedFileNames::ASSETS_REGISTRY_NAME;
        std::ofstream process(filePath);
        process.clear();
        process << YAML::Node(*this);
        process.close();
    }

    void AssetsConfig::deserializeConfig(const char *projectPath)
    {
        ZoneScoped;
        _projectPath = projectPath;
        const auto filePath = std::string(projectPath) + "\\" + ReservedFileNames::ASSETS_REGISTRY_NAME;
        const auto rawConfig = YAML::LoadFile(filePath);
        if (rawConfig.IsNull())
        {
            findAllAssets(projectPath);
            serialize();
            return;
        }

        const auto data = rawConfig.as<AssetsConfig>();
        _projectPath = data._projectPath;
        _rootFolder = data._rootFolder;
        buildIndexes();

        const FilePathList filesProvider = LoadDirectoryFiles(_projectPath.c_str());
        if (!isValid(filesProvider))
        {
            findAllAssets(projectPath);
            serialize();
        }

        UnloadDirectoryFiles(filesProvider);
    }

    void AssetsConfig::deserializeConfig()
    {
        ZoneScoped;
        deserializeConfig(TextFormat("%s\\%s", GetWorkingDirectory(), "assets\\game"));
    }

    void AssetsConfig::findAllAssets(const char *projectPath)
    {
        ZoneScoped;
        _projectPath = projectPath;
        _rootFolder = Folder(_projectPath, 0, "Project Files", "Project Files", "0");
        const FilePathList filesProvider = LoadDirectoryFiles(projectPath);
        parseFolders(_rootFolder, filesProvider);
        UnloadDirectoryFiles(filesProvider);
        buildIndexes();
    }

    bool AssetsConfig::isFolder(const char *path)
    {
        ZoneScoped;
        const auto paths = LoadDirectoryFiles(path);
        const auto isFolder = paths.count > 0 && GetFileExtension(path) == nullptr;
        UnloadDirectoryFiles(paths);
        return isFolder;
    }

    Folder *AssetsConfig::getRootFolder()
    {
        ZoneScoped;
        return &_rootFolder;
    }

    const std::string &AssetsConfig::getPathByGuid(const std::string &guid)
    {
        ZoneScoped;
        const auto folder_it = _guidToFolder.find(guid);
        if (folder_it != _guidToFolder.end())
        {
            return folder_it->second->_pathFromRoot;
        }

        const auto file_it = _guidToFile.find(guid);
        if (file_it != _guidToFile.end())
        {
            return file_it->second->_pathFromRoot;
        }

        return _empty;
    }

    const std::string &AssetsConfig::getGuidByPath(const std::string &path)
    {
        ZoneScoped;
        const auto folder_it = _pathToFolder.find(path);
        if (folder_it != _pathToFolder.end())
        {
            return folder_it->second->_guid;
        }

        const auto file_it = _pathToFile.find(path);
        if (file_it != _pathToFile.end())
        {
            return file_it->second->_guid;
        }

        return _empty;
    }

    File *AssetsConfig::getFileByGuid(const std::string &guid)
    {
        ZoneScoped;
        if (!_guidToFile.contains(guid)) return nullptr;
        return _guidToFile[guid];
    }

    Folder *AssetsConfig::getFolderByGuid(const std::string &guid)
    {
        ZoneScoped;
        if (guid == _rootFolder.getGUID()) return &_rootFolder;
        if (!_guidToFolder.contains(guid)) return nullptr;
        return _guidToFolder[guid];
    }

    File *AssetsConfig::getFileByPath(const std::string &path)
    {
        ZoneScoped;
        if (!_pathToFile.contains(path)) return nullptr;
        return _pathToFile[path];
    }

    Folder *AssetsConfig::getFolderByPath(const std::string &path)
    {
        ZoneScoped;
        if (path == _rootFolder.getFullPath()) return &_rootFolder;
        if (!_pathToFolder.contains(path)) return nullptr;
        return _pathToFolder[path];
    }

    void AssetsConfig::renameFile(const std::string &fileGuid, const std::string &nextName)
    {
        ZoneScoped;
        const auto file = _guidToFile[fileGuid];
        const auto oldPath = file->_fullPath;
        const auto folderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
        auto folder = _pathToFolder[folderPath];
        if (folder == nullptr) folder = &_rootFolder;

        file->_shortName = nextName;
        file->_fullPath = folder->getFullPath() + "\\" + file->getShortName();
        file->_pathFromRoot = folder->_pathFromRoot + "\\" + file->getShortName();
        buildIndexes();
        renameInternal(oldPath, file->_fullPath);
        serialize();
    }

    void AssetsConfig::renameFolder(const std::string &folderGuid, const std::string &nextName)
    {
        ZoneScoped;
        const auto folder = _guidToFolder[folderGuid];
        const auto oldPath = folder->_fullPath;
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        auto oldFolder = _pathToFolder[oldFolderPath];
        if (oldFolder == nullptr) oldFolder = &_rootFolder;

        folder->_name = nextName;
        if (folder->getDepth() == 0)
        {
            folder->_pathFromRoot = folder->getShortName();
        }
        else
        {
            folder->_pathFromRoot = oldFolder->_pathFromRoot + "\\" + folder->getShortName();
        }

        updateIncludesAfterFolderChange(folder);
        buildIndexes();
        renameInternal(oldPath, folder->_fullPath);
        serialize();
    }

    void AssetsConfig::moveFile(const std::string &fileGuid, const std::string &nextFolderGuid)
    {
        ZoneScoped;
        auto file = _guidToFile[fileGuid];
        const auto oldPath = file->_fullPath;
        const auto oldFolderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
        auto oldFolder = _pathToFolder[oldFolderPath];
        if (oldFolder == nullptr) oldFolder = &_rootFolder;
        const auto nextFolder = getFolderByGuid(nextFolderGuid);
        nextFolder->getFiles().emplace_back(std::move(*file));
        oldFolder->removeFile(file);

        file = &nextFolder->getFiles().back();
        file->_fullPath = nextFolder->getFullPath() + "\\" + file->getShortName();
        file->_pathFromRoot = nextFolder->_pathFromRoot + "\\" + file->getShortName();
        buildIndexes();
        moveInternal(oldPath, file->_fullPath);
        serialize();
    }

    void AssetsConfig::moveFolder(const std::string &folderGuid, const std::string &nextFolderGuid)
    {
        ZoneScoped;
        auto folder = _guidToFolder[folderGuid];
        const auto oldPath = folder->_fullPath;
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        auto oldFolder = _pathToFolder[oldFolderPath];
        if (oldFolder == nullptr) oldFolder = &_rootFolder;
        const auto &nextFolderGuid_copy = nextFolderGuid;
        const auto nextFolder = getFolderByGuid(nextFolderGuid);
        nextFolder->getFolders().emplace_back(std::move(*folder));
        oldFolder->removeFolder(folder);
        buildIndexes();

        folder = &getFolderByGuid(nextFolderGuid_copy)->getFolders().back();
        const auto nextFolder_updated = getFolderByGuid(nextFolderGuid_copy);
        folder->_depth = nextFolder_updated->getDepth() + 1;
        folder->_fullPath = nextFolder_updated->getFullPath() + "\\" + folder->_name;
        if (nextFolder_updated->getDepth() == 0)
        {
            folder->_pathFromRoot = folder->_name;
        }
        else
        {
            folder->_pathFromRoot = nextFolder_updated->_pathFromRoot + "\\" + folder->_name;
        }

        updateIncludesAfterFolderChange(folder);
        buildIndexes();
        moveInternal(oldPath, folder->_fullPath);
        serialize();
    }

    void AssetsConfig::deleteFile(const std::string &fileGuid)
    {
        ZoneScoped;
        const auto file = getFileByGuid(fileGuid);
        const auto oldFolderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
        auto oldFolder = _pathToFolder[oldFolderPath];
        if (oldFolder == nullptr) oldFolder = &_rootFolder;
        deleteFileInternal(file->_pathFromRoot);
        oldFolder->removeFile(file);
        buildIndexes();
        serialize();
    }

    void AssetsConfig::deleteFolder(const std::string &folderGuid)
    {
        ZoneScoped;
        const auto folder = getFolderByGuid(folderGuid);
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        auto oldFolder = _pathToFolder[oldFolderPath];
        if (oldFolder == nullptr) return;
        deleteFolderInternal(folder->_fullPath);
        oldFolder->removeFolder(folder);
        buildIndexes();
        serialize();
    }

    void AssetsConfig::updateIncludesAfterFolderChange(Folder *folder)
    {
        ZoneScoped;
        for (auto &file: folder->_files)
        {
            file._fullPath = folder->getFullPath() + "\\" + file.getShortName();
            file._pathFromRoot = folder->_pathFromRoot + "\\" + file.getShortName();
        }

        for (auto &childFolder: folder->_folders)
        {
            childFolder._depth = folder->getDepth() + 1;
            childFolder._fullPath = folder->getFullPath() + "\\" + childFolder.getShortName();
            childFolder._pathFromRoot = folder->_pathFromRoot + "\\" + childFolder.getShortName();
            updateIncludesAfterFolderChange(&childFolder);
        }
    }

    void AssetsConfig::buildIndexes()
    {
        ZoneScoped;
        _guidToFolder.clear();
        _guidToFile.clear();
        _pathToFolder.clear();
        _pathToFile.clear();
        indexFolder(_rootFolder);
    }

    void AssetsConfig::indexFolder(Folder &folder)
    {
        ZoneScoped;
        _guidToFolder[folder._guid] = &folder;
        _pathToFolder[folder._fullPath] = &folder;
        for (auto &file: folder._files)
        {
            _guidToFile[file._guid] = &file;
            _pathToFile[file._fullPath] = &file;
        }
        for (auto &subfolder: folder._folders)
        {
            indexFolder(subfolder);
        }
    }

    bool AssetsConfig::isValid(const FilePathList &filePathList)
    {
        ZoneScoped;
        for (auto i = 0u; i < filePathList.count; i++)
        {
            if (const auto path = filePathList.paths[i]; isFolder(path))
            {
                auto list = LoadDirectoryFiles(path);
                if (!isValid(list))
                {
                    UnloadDirectoryFiles(list);
                    return false;
                }

                UnloadDirectoryFiles(list);
            }
            else
            {
                if (_pathToFile[path] == nullptr)
                {
                    return false;
                }
            }
        }

        return true;
    }

    void AssetsConfig::parseFolders(Folder &folder, const FilePathList &filePathList) noexcept
    {
        ZoneScoped;
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = AssetsConfig::isFolder(path);
            if (strncmp(path, _projectPath.c_str(), _projectPath.size()) != 0)
            {
                Logger::LogError("Path does not start with project path");
                throw std::runtime_error("Path does not start with project path");
            }

            std::string relPath = path + _projectPath.size() + 1;
            if (isFolder)
            {
                auto subFolder = Folder(path, folder.getDepth() + 1, relPath, GetFileName(path), getNewGUID());
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

    void AssetsConfig::moveInternal(const std::string &from, const std::string &to)
    {
        ZoneScoped;
        try
        {
            if (fs::equivalent(fs::path(from).root_path(), fs::path(to).root_path()))
            {
                renameInternal(from, to);
                return;
            }

            fs::copy(from, to, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
            fs::remove_all(from);
        }
        catch (const fs::filesystem_error &err)
        {
            Logger::LogError(err.what());
        }
    }

    void AssetsConfig::copyFileInternal(const std::string &from, const std::string &to)
    {
        ZoneScoped;
        try
        {
            fs::copy(from, to, fs::copy_options::overwrite_existing);
        }
        catch (const fs::filesystem_error &err)
        {
            Logger::LogError(err.what());
        }
    }

    void AssetsConfig::copyFolderInternal(const std::string &from, const std::string &to)
    {
        ZoneScoped;
        try
        {
            fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
        catch (const fs::filesystem_error &err)
        {
            Logger::LogError(err.what());
        }
    }

    void AssetsConfig::renameInternal(const std::string &from, const std::string &to)
    {
        ZoneScoped;
        try
        {
            fs::rename(from, to);
        }
        catch (const fs::filesystem_error &err)
        {
            Logger::LogError(err.what());
        }
    }

    void AssetsConfig::deleteFileInternal(const std::string &from)
    {
        ZoneScoped;
        try
        {
            fs::remove(from);
        }
        catch (const fs::filesystem_error &err)
        {
            Logger::LogError(err.what());
        }
    }

    void AssetsConfig::deleteFolderInternal(const std::string &from)
    {
        ZoneScoped;
        try
        {
            fs::remove_all(from);
        }
        catch (const fs::filesystem_error &err)
        {
            Logger::LogError(err.what());
        }
    }
} // BreadEngine
