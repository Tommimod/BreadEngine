#include "assetsConfig.h"
#include <fstream>
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

    AssetsConfig::AssetsConfig()
    {
    }

    AssetsConfig::~AssetsConfig()
    {
        onIndirectChange.unsubscribeAll();
        _guidToAsset.clear();
        _guidToFolder.clear();
        _guidToFile.clear();
        _pathToGuid.clear();
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
        _projectPath = std::move(data._projectPath);
        _rootFolder = std::move(data._rootFolder);
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
        _rootFolder = std::make_shared<Folder>(_projectPath, 0, "Project Files", "Project Files", "0");
        const FilePathList filesProvider = LoadDirectoryFiles(projectPath);
        parseFolders(_rootFolder, filesProvider);
        UnloadDirectoryFiles(filesProvider);
        buildIndexes();
    }

    std::shared_ptr<Asset> AssetsConfig::getAsset(const std::shared_ptr<File> &file)
    {
        auto &guid = file->getGUID();
        if (_guidToAsset.contains(guid))
        {
            return _guidToAsset[guid];
        }

        if (file->is3DModel())
        {
            _guidToAsset[guid] = std::make_shared<MeshAsset>(guid);
        }
        else if (file->isImage())
        {
            _guidToAsset[guid] = std::make_shared<TextureAsset>(guid);
        }

        const auto asset = _guidToAsset[guid];
        if (asset == nullptr) return nullptr;
        asset->loadToMemory();
        return asset;
    }

    bool AssetsConfig::isFolder(const char *path)
    {
        ZoneScoped;
        const auto isFolder = GetFileExtension(path) == nullptr;
        return isFolder;
    }

    std::shared_ptr<Folder> AssetsConfig::getRootFolder() const
    {
        ZoneScoped;
        return _rootFolder;
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
        if (const auto it = _pathToGuid.find(path); it != _pathToGuid.end())
        {
            return it->second;
        }

        return _empty;
    }

    std::shared_ptr<File> AssetsConfig::getFileByGuid(const std::string &guid)
    {
        ZoneScoped;
        if (guid.empty() || !_guidToFile.contains(guid)) return nullptr;
        return _guidToFile[guid];
    }

    std::shared_ptr<Folder> AssetsConfig::getFolderByGuid(const std::string &guid)
    {
        ZoneScoped;
        if (guid == _rootFolder->getGUID()) return _rootFolder;
        if (guid.empty() || !_guidToFolder.contains(guid)) return nullptr;
        return _guidToFolder[guid];
    }

    std::shared_ptr<File> AssetsConfig::getFileByPath(const std::string &path)
    {
        ZoneScoped;
        const auto &guid = getGuidByPath(path);
        if (guid.empty()) return nullptr;
        return getFileByGuid(guid);
    }

    std::shared_ptr<Folder> AssetsConfig::getFolderByPath(const std::string &path)
    {
        ZoneScoped;
        const auto &guid = getGuidByPath(path);
        if (guid.empty()) return nullptr;
        return getFolderByGuid(guid);
    }

    void AssetsConfig::renameFile(const std::string &fileGuid, const std::string &nextName)
    {
        ZoneScoped;
        const auto file = _guidToFile[fileGuid];
        const auto oldPath = file->_fullPath;
        const auto folderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
        auto folder = getFolderByPath(folderPath);
        if (folder == nullptr) folder = _rootFolder;

        file->_shortName = nextName;
        file->_fullPath = folder->getFullPath() + "\\" + file->getShortName();
        file->_pathFromRoot = folder->_pathFromRoot + "\\" + file->getShortName();
        buildIndexes();
        renameInternal(oldPath, file->_fullPath);
        Logger::LogInfo("File renamed: " + oldPath + " -> " + file->_fullPath);
        serialize();
    }

    void AssetsConfig::renameFolder(const std::string &folderGuid, const std::string &nextName)
    {
        ZoneScoped;
        const auto folder = _guidToFolder[folderGuid];
        const auto oldPath = folder->_fullPath;
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        auto oldFolder = getFolderByPath(oldFolderPath);
        if (oldFolder == nullptr) oldFolder = _rootFolder;

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
        Logger::LogInfo("Folder renamed: " + oldPath + " -> " + folder->_fullPath);
        serialize();
    }

    void AssetsConfig::moveFile(const std::string &fileGuid, const std::string &nextFolderGuid, const bool withInternalOperations)
    {
        ZoneScoped;
        auto file = _guidToFile[fileGuid];
        const auto oldPath = file->_fullPath;
        const auto oldFolderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
        auto oldFolder = getFolderByPath(oldFolderPath);
        if (oldFolder == nullptr) oldFolder = _rootFolder;

        const auto nextFolder = getFolderByGuid(nextFolderGuid);
        nextFolder->getFiles().emplace_back(std::move(file));
        oldFolder->removeFile(file);

        file = nextFolder->getFiles().back();
        file->_fullPath = nextFolder->getFullPath() + "\\" + file->getShortName();
        file->_pathFromRoot = nextFolder->_pathFromRoot + "\\" + file->getShortName();
        if (!withInternalOperations)
        {
            return;
        }

        buildIndexes();
        moveInternal(oldPath, file->_fullPath);
        Logger::LogInfo("File moved: " + oldPath + " -> " + file->_fullPath);
        serialize();
    }

    void AssetsConfig::moveFolder(const std::string &folderGuid, const std::string &nextFolderGuid, const bool withInternalOperations)
    {
        ZoneScoped;
        auto folder = _guidToFolder[folderGuid];
        const auto oldPath = folder->_fullPath;
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        auto oldFolder = getFolderByPath(oldFolderPath);
        if (oldFolder == nullptr) oldFolder = _rootFolder;

        const auto &nextFolderGuid_copy = nextFolderGuid;
        const auto nextFolder = getFolderByGuid(nextFolderGuid);
        nextFolder->getFolders().emplace_back(std::move(folder));
        oldFolder->removeFolder(folder);
        buildIndexes();

        folder = getFolderByGuid(nextFolderGuid_copy)->getFolders().back();
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
        if (!withInternalOperations)
        {
            return;
        }

        buildIndexes();
        moveInternal(oldPath, folder->_fullPath);
        Logger::LogInfo("Folder moved: " + oldPath + " -> " + folder->_fullPath);
        serialize();
    }

    void AssetsConfig::deleteFile(const std::string &fileGuid, const bool withInternalOperations)
    {
        ZoneScoped;
        const auto file = getFileByGuid(fileGuid);
        const auto oldFolderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
        auto oldFolder = getFolderByPath(oldFolderPath);
        if (oldFolder == nullptr) oldFolder = _rootFolder;
        oldFolder->removeFile(file);
        if (_guidToAsset.contains(fileGuid))
        {
            _guidToAsset.erase(fileGuid);
        }

        if (!withInternalOperations)
        {
            return;
        }

        deleteFileInternal(file->getFullPath());
        Logger::LogInfo("File deleted: " + file->getPathFromRoot());
        buildIndexes();
        serialize();
    }

    void AssetsConfig::deleteFolder(const std::string &folderGuid, const bool withInternalOperations)
    {
        ZoneScoped;
        const auto folder = getFolderByGuid(folderGuid);
        const auto oldFolderPath = std::string(GetPrevDirectoryPath(folder->getFullPath().c_str()));
        auto oldFolder = getFolderByPath(oldFolderPath);
        if (oldFolder == nullptr) oldFolder = _rootFolder;

        for (const auto &childFolder: folder->getFolders())
        {
            deleteFolder(childFolder->getGUID(), withInternalOperations);
        }

        for (const auto &file: folder->getFiles())
        {
            if (_guidToAsset.contains(file->getGUID()))
            {
                _guidToAsset.erase(file->getGUID());
            }
        }

        oldFolder->removeFolder(folder);
        if (!withInternalOperations)
        {
            return;
        }

        deleteFolderInternal(folder->getFullPath());
        Logger::LogInfo("Folder deleted: " + folder->getPathFromRoot());
        buildIndexes();
        serialize();
    }

    void AssetsConfig::onEntityCreated(const std::string &filePath)
    {
        auto prevFolder = getFolderByPath(GetDirectoryPath(filePath.c_str()));
        prevFolder = prevFolder ? prevFolder : _rootFolder;
        const auto isFolder = AssetsConfig::isFolder(filePath.c_str());
        const auto ext = GetFileExtension(filePath.c_str());
        const auto guid = getNewGUID();
        const auto relPath = filePath.c_str() + _projectPath.size() + 1;
        if (isFolder)
        {
            auto folder = std::make_shared<Folder>(filePath, prevFolder->getDepth() + 1, relPath, GetFileName(filePath.c_str()), guid);
            prevFolder->getFolders().emplace_back(std::move(folder));
            const auto &link = prevFolder->getFolders().back();
            _pathToGuid[filePath] = guid;
            _guidToFolder[guid] = link;
            Logger::LogInfo("Folder created: " + link->getPathFromRoot());
        }
        else
        {
            auto file = std::make_shared<File>(filePath, relPath, ext, guid);
            prevFolder->getFiles().emplace_back(std::move(file));
            const auto &link = prevFolder->getFiles().back();
            _pathToGuid[filePath] = guid;
            _guidToFile[guid] = link;
            getAsset(link);
            Logger::LogInfo("File created: " + link->getPathFromRoot());
        }

        buildIndexes();
        serializeConfig();
        onIndirectChange.invoke();
    }

    void AssetsConfig::onEntityDeleted(const std::string &filePath)
    {
        if (isFolder(filePath.c_str()))
        {
            if (!_pathToGuid.contains(filePath)) return;
            const auto &guid = _pathToGuid[filePath];
            deleteFolder(guid.data(), false);
        }
        else
        {
            if (!_pathToGuid.contains(filePath)) return;
            const auto &guid = _pathToGuid[filePath];
            deleteFile(guid.data(), false);
        }

        buildIndexes();
        serializeConfig();
        onIndirectChange.invoke();
    }

    void AssetsConfig::onEntityMoved(const std::string &from, const std::string &to)
    {
        if (isFolder(from.c_str()))
        {
            if (!_pathToGuid.contains(from)) return;
            const auto &fromGuid = _pathToGuid[from];
            const auto nextFolder = getFolderByPath(GetPrevDirectoryPath(to.c_str()));
            if (nextFolder->contains(fromGuid.data())) return;
            auto &toGuid = nextFolder->getGUID();
            moveFolder(fromGuid.data(), toGuid, false);
        }
        else
        {
            if (!_pathToGuid.contains(from)) return;
            const auto &fromGuid = _pathToGuid[from];
            const auto nextFolder = getFolderByPath(GetPrevDirectoryPath(to.c_str()));
            if (nextFolder->contains(fromGuid.data())) return;
            auto &toGuid = nextFolder->getGUID();
            moveFile(fromGuid.data(), toGuid, false);
        }

        buildIndexes();
        serializeConfig();
        onIndirectChange.invoke();
    }

    void AssetsConfig::updateIncludesAfterFolderChange(const std::shared_ptr<Folder> &folder)
    {
        ZoneScoped;
        for (const auto &file: folder->_files)
        {
            file->_fullPath = folder->getFullPath() + "\\" + file->getShortName();
            file->_pathFromRoot = folder->_pathFromRoot + "\\" + file->getShortName();
        }

        for (auto &childFolder: folder->_folders)
        {
            childFolder->_depth = folder->getDepth() + 1;
            childFolder->_fullPath = folder->getFullPath() + "\\" + childFolder->getShortName();
            childFolder->_pathFromRoot = folder->_pathFromRoot + "\\" + childFolder->getShortName();
            updateIncludesAfterFolderChange(childFolder);
        }
    }

    struct stat info;

    void AssetsConfig::removeUndefinedFoldersAndFiles()
    {
        auto pathForRemove = std::vector<std::string>{};
        auto guidForRemove = std::vector<std::string>{};
        for (auto &[guid, folder]: _guidToFolder)
        {
            auto &path = getPathByGuid(guid.data());
            if (stat(path.data(), &info) != 0)
            {
            }
            else if (info.st_mode & S_IFDIR) continue;
            pathForRemove.emplace_back(path);
            guidForRemove.emplace_back(guid);
            auto parentFolder = getFolderByPath(GetPrevDirectoryPath(path.data()));
            parentFolder = parentFolder ? parentFolder : _rootFolder;
            parentFolder->removeFolder(folder);
        }

        for (const auto &guid: guidForRemove)
        {
            _guidToFolder.erase(guid);
        }

        guidForRemove.clear();
        for (auto &[guid, file]: _guidToFile)
        {
            auto &path = getPathByGuid(guid.data());
            if (FileExists(path.data())) continue;
            pathForRemove.emplace_back(path);
            guidForRemove.emplace_back(guid);
            auto parentFolder = getFolderByPath(GetPrevDirectoryPath(path.data()));
            parentFolder = parentFolder ? parentFolder : _rootFolder;
            parentFolder->removeFile(file);
        }

        for (const auto &path: pathForRemove)
        {
            _pathToGuid.erase(path);
        }

        for (const auto &guid: guidForRemove)
        {
            _guidToFile.erase(guid);
        }
    }

    void AssetsConfig::restoreProjectTree(std::shared_ptr<Folder> &folder, const FilePathList &filePathList)
    {
        ZoneScoped;
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = AssetsConfig::isFolder(path);
            if (!isFolder && getFileByPath(path) != nullptr)
            {
                continue;
            }

            std::string relPath = path + _projectPath.size() + 1;
            if (auto folderByGuid = getFolderByPath(path); isFolder && folderByGuid == nullptr)
            {
                auto guid = getNewGUID();
                auto subFolder = std::make_shared<Folder>(path, folder->getDepth() + 1, relPath, GetFileName(path), guid);
                auto list = LoadDirectoryFiles(path);
                restoreProjectTree(subFolder, list);
                folder->getFolders().emplace_back(std::move(subFolder));
                const auto &link = folder->getFolders().back();
                _pathToGuid[path] = guid;
                _guidToFolder[guid] = link;
                UnloadDirectoryFiles(list);
            }
            else if (isFolder && folderByGuid != nullptr)
            {
                auto list = LoadDirectoryFiles(path);
                restoreProjectTree(folderByGuid, list);
                UnloadDirectoryFiles(list);
            }
            else if (!isFolder)
            {
                const auto ext = GetFileExtension(path);
                auto guid = getNewGUID();
                auto file = std::make_shared<File>(path, relPath, ext, guid);
                folder->getFiles().emplace_back(std::move(file));
                const auto &link = folder->getFiles().back();
                _pathToGuid[path] = guid;
                _guidToFile[guid] = link;
                getAsset(link);
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

            if (file->isImage()) textures.emplace_back(asset.get());
            if (file->is3DModel()) models.emplace_back(asset.get());
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
            if (file == nullptr) continue;
            if (file->isImage()) textures.emplace_back(asset.get());
            if (file->is3DModel()) models.emplace_back(asset.get());
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
        _pathToGuid.clear();
        indexFolder(_rootFolder);
    }

    void AssetsConfig::indexFolder(const std::shared_ptr<Folder> &folder)
    {
        ZoneScoped;
        _guidToFolder.emplace(folder->_guid, folder);
        _pathToGuid.emplace(folder->_fullPath, folder->_guid);
        for (auto &file: folder->_files)
        {
            _guidToFile.emplace(file->_guid, file);
            _pathToGuid.emplace(file->_fullPath, file->_guid);
            Logger::LogInfo("File indexed: " + file->_fullPath + "; guid: " + file->_guid);
        }
        for (auto &subfolder: folder->_folders)
        {
            indexFolder(subfolder);
        }
    }

    void AssetsConfig::parseFolders(const std::shared_ptr<Folder> &folder, const FilePathList &filePathList)
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
                auto subFolder = std::make_shared<Folder>(path, folder->getDepth() + 1, relPath, GetFileName(path), getNewGUID());
                auto list = LoadDirectoryFiles(path);
                parseFolders(subFolder, list);
                folder->getFolders().push_back(std::move(subFolder));
                UnloadDirectoryFiles(list);
            }
            else
            {
                const auto ext = GetFileExtension(path);
                auto file = std::make_shared<File>(path, relPath, ext, getNewGUID());
                folder->getFiles().push_back(std::move(file));
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
