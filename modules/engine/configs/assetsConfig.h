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
        Action<> onIndirectChange{};

        AssetsConfig() = default;

        ~AssetsConfig() override;

        void serializeConfig() override;

        void deserializeConfig(const char *filePath) override;

        void buildFullProjectTree(const char *projectPath);

        Asset *getAsset(const File *file);

        [[nodiscard]] static bool isFolder(const char *path);

        [[nodiscard]] Folder *getRootFolder();

        [[nodiscard]] const std::string &getPathByGuid(const std::string &guid);

        [[nodiscard]] const std::string &getGuidByPath(const std::string &path);

        [[nodiscard]] File *getFileByGuid(const std::string &guid);

        [[nodiscard]] Folder *getFolderByGuid(const std::string &guid);

        [[nodiscard]] File *getFileByPath(const std::string &path);

        [[nodiscard]] Folder *getFolderByPath(const std::string &path);

        void renameFile(const std::string &fileGuid, const std::string &nextName);

        void renameFolder(const std::string &folderGuid, const std::string &nextName);

        void moveFile(const std::string &fileGuid, const std::string &nextFolderGuid);

        void moveFolder(const std::string &folderGuid, const std::string &nextFolderGuid);

        void deleteFile(const std::string &fileGuid);

        void deleteFolder(const std::string &folderGuid);

        bool operator==(const AssetsConfig &other) const
        {
            return _projectPath == other._projectPath && _rootFolder == other._rootFolder;
        }

        void onEntityCreated(const std::string &filePath);

        void onEntityDeleted(const std::string &filePath);

        void onEntityMoved(const std::string &from, const std::string &to);

    private:
        friend struct YAML::convert<AssetsConfig>;
        friend class AssetsDeserializer;

        Folder _rootFolder;
        std::unordered_map<std::string_view, Folder *> _guidToFolder{};
        std::unordered_map<std::string_view, Folder *> _pathToFolder{};
        std::unordered_map<std::string_view, File *> _guidToFile{};
        std::unordered_map<std::string_view, File *> _pathToFile{};
        std::map<std::string, Asset *> _guidToAsset{};
        std::string _empty;
        std::string _projectPath;
        std::string _filePath;

        static void updateIncludesAfterFolderChange(Folder *folder);

        void removeUndefinedFoldersAndFiles();

        void restoreProjectTree(Folder &folder, const FilePathList &filePathList);

        void restoreEngineAssetsByFiles(bool withInitialize);

        void initializeExistingEngineAssets();

        void buildIndexes();

        void indexFolder(Folder &folder);

        void parseFolders(Folder &folder, const FilePathList &filePathList);

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
