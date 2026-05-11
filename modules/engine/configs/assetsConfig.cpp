#include "assetsConfig.h"
#include <fstream>
#include <ranges>
#include <sys/stat.h>

#include "raylib.h"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include "assetsConfigYaml.h"
#include "assetsDeserializer.h"
#include "logger.h"
#include "meshAsset.h"
#include "textureAsset.h"
#include "tracy/Tracy.hpp"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(AssetsConfig)
    namespace fs = std::filesystem;

    AssetsConfig::~AssetsConfig()
    {
        onIndirectChange.unsubscribeAll();
        for (const auto asset: _guidToAsset | std::views::values)
        {
            delete asset;
        }

        _guidToAsset.clear();
        _guidToFolder.clear();
        _guidToFile.clear();
        _pathToFolder.clear();
        _pathToFile.clear();
    }

    void AssetsConfig::serializeConfig()
    {
        ZoneScoped;
        std::ofstream process(_filePath);
        process.clear();
        process << YAML::Node(*this);
        process.close();
    }

    void AssetsConfig::deserializeConfig(const char *filePath)
    {
        ZoneScoped;
        _filePath = filePath;
        _projectPath = GetDirectoryPath(filePath);
        auto rawConfig = YAML::LoadFile(_filePath);
        if (rawConfig.IsNull())
        {
            buildFullProjectTree(_projectPath.c_str());
            restoreEngineAssetsByFiles(true);
            serializeConfig();
            return;
        }

        const auto data = rawConfig.as<AssetsConfig>();
        _projectPath = data._projectPath;
        _rootFolder = data._rootFolder;
        buildIndexes();
        AssetsDeserializer::deserialize(rawConfig);

        const FilePathList filesProvider = LoadDirectoryFiles(_projectPath.c_str());
        removeUndefinedFoldersAndFiles();
        restoreProjectTree(_rootFolder, filesProvider);
        UnloadDirectoryFiles(filesProvider);

        serializeConfig();
        buildIndexes();
        restoreEngineAssetsByFiles(false);
        initializeExistingEngineAssets();
    }

    void AssetsConfig::buildFullProjectTree(const char *projectPath)
    {
        ZoneScoped;
        _projectPath = projectPath;
        _rootFolder = Folder(_projectPath, 0, "Project Files", "Project Files", "0");
        const FilePathList filesProvider = LoadDirectoryFiles(projectPath);
        parseFolders(_rootFolder, filesProvider);
        UnloadDirectoryFiles(filesProvider);
        buildIndexes();
    }

    Asset *AssetsConfig::getAsset(const File *file)
    {
        auto &guid = file->getGUID();
        if (_guidToAsset.contains(guid))
        {
            return _guidToAsset[guid];
        }

        if (file->is3DModel())
        {
            _guidToAsset[guid] = new MeshAsset(guid);
        }
        else if (file->isImage())
        {
            _guidToAsset[guid] = new TextureAsset(guid);
        }

        const auto asset = _guidToAsset[guid];
        if (asset == nullptr) return nullptr;
        asset->loadToMemory();
        return asset;
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
        deleteFileInternal(file->getFullPath());
        oldFolder->removeFile(file);
        if (_guidToAsset.contains(fileGuid))
        {
            delete _guidToAsset[fileGuid];
            _guidToAsset.erase(fileGuid);
        }

        buildIndexes();
        serialize();
    }

    void AssetsConfig::deleteFolder(const std::string &folderGuid)
    {
        ZoneScoped;
        const auto folder = getFolderByGuid(folderGuid);
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        const auto oldFolder = _pathToFolder[oldFolderPath];
        if (oldFolder == nullptr) return;
        const auto &files = folder->getFiles();
        for (auto &file: files)
        {
            if (_guidToAsset.contains(file.getGUID()))
            {
                delete _guidToAsset[file.getGUID()];
                _guidToAsset.erase(file.getGUID());
            }
        }

        deleteFolderInternal(folder->getFullPath());
        oldFolder->removeFolder(folder);
        buildIndexes();
        serialize();
    }

    void AssetsConfig::onEntityCreated(const std::string &filePath)
    {
        auto prevFolder = _pathToFolder[GetDirectoryPath(filePath.c_str())];
        prevFolder = prevFolder ? prevFolder : &_rootFolder;
        const auto isFolder = AssetsConfig::isFolder(filePath.c_str());
        const auto ext = GetFileExtension(filePath.c_str());
        const auto guid = getNewGUID();
        const auto relPath = filePath.c_str() + _projectPath.size() + 1;
        if (isFolder)
        {
            auto folder = Folder(filePath.c_str(), prevFolder->getDepth() + 1, relPath, GetFileName(filePath.c_str()), guid);
            prevFolder->getFolders().emplace_back(std::move(folder));
            auto &link = prevFolder->getFolders().back();
            _pathToFolder[filePath] = &link;
            _guidToFolder[guid] = &link;
        }
        else
        {
            auto file = File(filePath.c_str(), relPath, ext, guid);
            prevFolder->getFiles().emplace_back(std::move(file));
            auto &link = prevFolder->getFiles().back();
            _pathToFile[filePath] = &link;
            _guidToFile[guid] = &link;
            getAsset(&link);
        }

        buildIndexes();
        serializeConfig();
        onIndirectChange.invoke();
    }

    void AssetsConfig::onEntityDeleted(const std::string &filePath)
    {
    }

    void AssetsConfig::onEntityMoved(const std::string &from, const std::string &to)
    {
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

    struct stat info;

    void AssetsConfig::removeUndefinedFoldersAndFiles()
    {
        auto pathForRemove = std::vector<std::string_view>{};
        auto guidForRemove = std::vector<std::string_view>{};
        for (auto &[path, folder]: _pathToFolder)
        {
            if (stat(path.data(), &info) != 0)
            {
            }
            else if (info.st_mode & S_IFDIR) continue;
            pathForRemove.emplace_back(path);
            guidForRemove.emplace_back(folder->getGUID());
            auto parentFolder = _pathToFolder[GetPrevDirectoryPath(path.data())];
            parentFolder = parentFolder ? parentFolder : &_rootFolder;
            parentFolder->removeFolder(folder);
        }

        for (const auto &path: pathForRemove)
        {
            _pathToFolder.erase(path);
        }

        for (const auto &guid: guidForRemove)
        {
            _guidToFolder.erase(guid);
        }

        pathForRemove.clear();
        guidForRemove.clear();
        for (auto &[path, file]: _pathToFile)
        {
            if (FileExists(path.data())) continue;
            pathForRemove.emplace_back(path);
            guidForRemove.emplace_back(file->getGUID());
            auto parentFolder = _pathToFolder[GetPrevDirectoryPath(path.data())];
            parentFolder = parentFolder ? parentFolder : &_rootFolder;
            parentFolder->removeFile(file);
        }

        for (const auto &path: pathForRemove)
        {
            _pathToFile.erase(path);
        }

        for (const auto &guid: guidForRemove)
        {
            _guidToFile.erase(guid);
        }
    }

    void AssetsConfig::restoreProjectTree(Folder &folder, const FilePathList &filePathList)
    {
        ZoneScoped;
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = AssetsConfig::isFolder(path);
            if (!isFolder && _pathToFile.contains(path))
            {
                continue;
            }

            if (strncmp(path, _projectPath.c_str(), _projectPath.size()) != 0)
            {
                Logger::LogError("Path does not start with project path");
                throw std::runtime_error("Path does not start with project path");
            }

            std::string relPath = path + _projectPath.size() + 1;
            if (isFolder && !_pathToFolder.contains(path))
            {
                auto guid = getNewGUID();
                auto subFolder = Folder(path, folder.getDepth() + 1, relPath, GetFileName(path), guid);
                auto list = LoadDirectoryFiles(path);
                restoreProjectTree(subFolder, list);
                folder.getFolders().emplace_back(std::move(subFolder));
                auto &link = folder.getFolders().back();
                _pathToFolder[path] = &link;
                _guidToFolder[guid] = &link;
                UnloadDirectoryFiles(list);
            }
            else if (isFolder && _pathToFolder.contains(path))
            {
                auto list = LoadDirectoryFiles(path);
                restoreProjectTree(*_pathToFolder[path], list);
                UnloadDirectoryFiles(list);
            }
            else if (!isFolder)
            {
                const auto ext = GetFileExtension(path);
                auto guid = getNewGUID();
                auto file = File(path, relPath, ext, guid);
                folder.getFiles().emplace_back(std::move(file));
                auto &link = folder.getFiles().back();
                _pathToFile[path] = &link;
                _guidToFile[guid] = &link;
            }
        }
    }

    void AssetsConfig::restoreEngineAssetsByFiles(const bool withInitialize)
    {
        std::vector<Asset *> textures;
        std::vector<Asset *> models;
        for (auto &[guid, file]: _guidToFile)
        {
            if (_guidToAsset.contains(guid.data())) continue;
            auto asset = getAsset(file);
            if (!withInitialize) continue;

            if (file->isImage()) textures.push_back(asset);
            if (file->is3DModel()) models.push_back(asset);
        }

        if (!withInitialize) return;
        for (const auto asset: textures)
        {
            asset->loadToMemory();
        }

        for (const auto asset: models)
        {
            asset->loadToMemory();
        }
    }

    void AssetsConfig::initializeExistingEngineAssets()
    {
        std::vector<Asset *> textures;
        std::vector<Asset *> models;
        for (auto &[guid, asset]: _guidToAsset)
        {
            if (asset == nullptr) continue;
            const auto file = _guidToFile[guid.data()];
            if (file->isImage()) textures.push_back(asset);
            if (file->is3DModel()) models.push_back(asset);
        }

        for (const auto asset: textures)
        {
            asset->loadToMemory();
        }

        for (const auto asset: models)
        {
            asset->loadToMemory();
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

    void AssetsConfig::parseFolders(Folder &folder, const FilePathList &filePathList)
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
