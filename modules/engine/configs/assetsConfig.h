#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#include "baseYamlConfig.h"
#include "raylib.h"
#include <unordered_map>
#include <string_view>
#include "action.h"
#include "file.h"
#include "folder.h"

namespace BreadEngine {
    struct AssetsConfig : BaseYamlConfig
    {
        bool operator==(const AssetsConfig &other) const
        {
            return _projectPath == other._projectPath && _rootFolder == other._rootFolder;
        }

        Action<> onIndirectChange{};

        AssetsConfig();

        ~AssetsConfig() override;

        void serializeConfig() override;

        void deserializeConfig(const char *filePath) override;

        void buildFullProjectTree(const char *projectPath);

        std::shared_ptr<Asset> getAsset(const std::shared_ptr<File> &file);

        [[nodiscard]] static bool isFolder(const char *path);

        [[nodiscard]] std::shared_ptr<Folder> getRootFolder() const;

        [[nodiscard]] const std::string &getPathByGuid(const std::string &guid);

        [[nodiscard]] const std::string &getGuidByPath(const std::string &path);

        static std::string normalizePathSeparators(const std::string &path);

        [[nodiscard]] std::shared_ptr<File> getFileByGuid(const std::string &guid);

        [[nodiscard]] std::shared_ptr<Folder> getFolderByGuid(const std::string &guid);

        [[nodiscard]] std::shared_ptr<File> getFileByPath(const std::string &path);

        [[nodiscard]] std::shared_ptr<Folder> getFolderByPath(const std::string &path);

        void renameFile(const std::string &fileGuid, const std::string &nextName);

        void renameFolder(const std::string &folderGuid, const std::string &nextName);

        void moveFile(const std::string &fileGuid, const std::string &nextFolderGuid, bool withInternalOperations = true);

        void moveFolder(const std::string &folderGuid, const std::string &nextFolderGuid, bool withInternalOperations = true);

        void deleteFile(const std::string &fileGuid, bool withInternalOperations = true);

        void deleteFolder(const std::string &folderGuid, bool withInternalOperations = true);

        void onEntityCreated(std::string &filePath);

        void onEntityDeleted(std::string &filePath);

        void onEntityMoved(std::string &from, std::string &to);

    private:
        friend struct YAML::convert<AssetsConfig>;
        friend class AssetsDeserializer;

        std::shared_ptr<Folder> _rootFolder;
        std::unordered_map<std::string, std::shared_ptr<Folder> > _guidToFolder{};
        std::unordered_map<std::string, std::shared_ptr<File> > _guidToFile{};
        std::unordered_map<std::string, std::string> _pathToGuid{};
        std::unordered_map<std::string, std::shared_ptr<Asset> > _guidToAsset{};
        std::string _empty;
        std::string _projectPath;
        std::string _filePath;

        static void updateIncludesAfterFolderChange(const std::shared_ptr<Folder> &folder);

        void removeUndefinedFoldersAndFiles();

        void restoreProjectTree(const std::shared_ptr<Folder> &folder, const FilePathList &filePathList);

        void restoreEngineAssetsByFiles(bool withInitialize);

        void initializeExistingEngineAssets();

        void buildIndexes();

        void indexFolder(const std::shared_ptr<Folder> &folder);

        void parseFolders(const std::shared_ptr<Folder> &folder, const FilePathList &filePathList);

        static void moveInternal(const std::string &from, const std::string &to);

        static void copyFileInternal(const std::string &from, const std::string &to);

        static void copyFolderInternal(const std::string &from, const std::string &to);

        static void renameInternal(const std::string &from, const std::string &to);

        static void deleteFileInternal(const std::string &from);

        static void deleteFolderInternal(const std::string &from);

        INSPECTOR_BEGIN(AssetsConfig)
            INSPECT_FIELD(_projectPath);
            INSPECT_FIELD(_rootFolder);
        INSPECTOR_END()
    };
} // BreadEngine
