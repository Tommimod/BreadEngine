#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#include "baseYamlConfig.h"
#include "nameof.h"
#include "raylib.h"
#include <unordered_map>
#include <string_view>

namespace BreadEngine {
    struct File : InspectorStruct
    {
        File() = default;

        File(const std::string &fullName, const std::string &pathFromRoot, const std::string &extension, const std::string &guid)
        {
            this->_fullPath = fullName;
            this->_pathFromRoot = pathFromRoot;
            this->_extension = extension;
            this->_guid = guid;
            this->_shortName = GetFileName(_fullPath.c_str());
        }

        ~File() override = default;

        bool operator==(const File &other) const
        {
            return _guid == other._guid && _fullPath == other._fullPath;
        }

        bool operator!=(const File &other) const
        {
            return !(*this == other);
        }

        bool operator==(const File *other) const
        {
            return _guid == other->_guid && _fullPath == other->_fullPath;
        }

        bool operator!=(const File *other) const
        {
            return this != other;
        }

        [[nodiscard]] const std::string &getGUID() const { return _guid; }
        [[nodiscard]] const std::string &getFullName() const { return _fullPath; }
        [[nodiscard]] const std::string &getPathFromRoot() const { return _pathFromRoot; }
        [[nodiscard]] const std::string &getExtension() const { return _extension; }
        [[nodiscard]] const std::string &getShortName() const { return _shortName; }

    private:
        friend struct YAML::convert<File>;
        friend struct AssetsConfig;
        std::string _guid;
        std::string _fullPath;
        std::string _pathFromRoot;
        std::string _extension;
        std::string _shortName;

        INSPECTOR_BEGIN(File)
            INSPECT_FIELD(_guid);
            INSPECT_FIELD(_fullPath);
            INSPECT_FIELD(_shortName);
            INSPECT_FIELD(_extension);
        INSPECTOR_END()
    };

    struct Folder : InspectorStruct
    {
        Folder() = default;

        Folder(const std::string &fullPath, const int depth, const std::string &pathFromRoot, const std::string &folderName, const std::string &guid)
        {
            this->_fullPath = fullPath;
            this->_depth = depth;
            this->_pathFromRoot = pathFromRoot;
            this->_guid = guid;
            this->_name = folderName;
        }

        ~Folder() override = default;

        [[nodiscard]] bool isEmpty() const { return _files.empty() && _folders.empty(); }
        [[nodiscard]] int getDepth() const { return _depth; }
        [[nodiscard]] const std::string &getGUID() const { return _guid; }
        [[nodiscard]] const std::string &getFullName() const { return _fullPath; }
        [[nodiscard]] const std::string &getPathFromRoot() const { return _pathFromRoot; }
        [[nodiscard]] const std::string &getShortName() const { return _name; }
        [[nodiscard]] std::vector<File> &getFiles() { return _files; }
        [[nodiscard]] std::vector<Folder> &getFolders() { return _folders; }

        bool operator==(const Folder &other) const
        {
            return _guid == other._guid && _fullPath == other._fullPath;
        }

        bool operator!=(const Folder &other) const
        {
            return !(*this == other);
        }

        void removeFile(const File *file)
        {
            auto it = std::ranges::find_if(_files, [&](const File &f)
            {
                return f.getGUID() == file->getGUID();
            });
            if (it != _files.end())
            {
                _files.erase(it);
            }
        }

        void removeFolder(const Folder *folder)
        {
            auto it = std::ranges::find_if(_folders, [&](const Folder &f)
            {
                return f.getGUID() == folder->getGUID();
            });
            if (it != _folders.end())
            {
                _folders.erase(it);
            }
        }

        bool tryFindRecursive(const Folder *folder)
        {
            for (auto &child: _folders)
            {
                if (child.getGUID() == folder->getGUID())
                {
                    return true;
                }

                if (child.tryFindRecursive(folder))
                {
                    return true;
                }
            }

            return false;
        }

        bool tryFindRecursive(const File *file)
        {
            for (auto &childFile: _files)
            {
                if (childFile.getGUID() == file->getGUID())
                {
                    return true;
                }
            }

            for (auto &childFolder: _folders)
            {
                if (childFolder.tryFindRecursive(file))
                {
                    return true;
                }
            }

            return false;
        }

        bool contains(const std::string &guid) const
        {
            for (auto &child: _files)
            {
                if (child.getGUID() == guid)
                {
                    return true;
                }
            }

            for (auto &child: _folders)
            {
                if (child.getGUID() == guid)
                {
                    return true;
                }
            }

            return false;
        }

    private:
        friend struct YAML::convert<Folder>;
        friend struct AssetsConfig;
        int _depth = 0;
        std::string _fullPath;
        std::string _guid;
        std::string _pathFromRoot;
        std::string _name;
        std::vector<File> _files;
        std::vector<Folder> _folders;

        INSPECTOR_BEGIN(Folder)
            INSPECT_FIELD(_depth);
            INSPECT_FIELD(_guid);
            INSPECT_FIELD(_pathFromRoot);
            INSPECT_FIELD(_files);
            INSPECT_FIELD(_folders);
        INSPECTOR_END()
    };

    struct AssetsConfig : BaseYamlConfig
    {
        AssetsConfig() = default;

        ~AssetsConfig() override = default;

        void serialize() override;

        void deserialize(const char *projectPath);

        void deserialize() override;

        void findAllAssets(const char *projectPath);

        [[nodiscard]] static bool isFolder(const char *path);

        [[nodiscard]] Folder *getRootFolder();

        [[nodiscard]] const std::string &getPathByGuid(const std::string &guid);

        [[nodiscard]] const std::string &getGuidByPath(const std::string &path);

        [[nodiscard]] File *getFileByGuid(const std::string &guid);

        [[nodiscard]] Folder *getFolderByGuid(const std::string &guid);

        void moveFile(const std::string &fileGuid, const std::string &nextFolderGuid);

        void moveFolder(const std::string &folderGuid, const std::string &nextFolderGuid);

        void deleteFile(const std::string &fileGuid);

        void deleteFolder(const std::string &folderGuid);

        bool operator==(const AssetsConfig &other) const
        {
            return _projectPath == other._projectPath && _rootFolder == other._rootFolder;
        }

    private:
        friend struct YAML::convert<AssetsConfig>;
        std::string _empty;
        std::string _projectPath;
        Folder _rootFolder = Folder(GetApplicationDirectory(), 0, "Project Files", "Project Files", "0");
        std::unordered_map<std::string_view, Folder *> _guidToFolder{};
        std::unordered_map<std::string_view, File *> _guidToFile{};
        std::unordered_map<std::string_view, Folder *> _pathToFolder{};
        std::unordered_map<std::string_view, File *> _pathToFile{};

        static void updateIncludesAfterMove(Folder *folder);

        void buildIndexes();

        void indexFolder(Folder &folder);

        void parseFolders(Folder &folder, const FilePathList &filePathList) noexcept;

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
