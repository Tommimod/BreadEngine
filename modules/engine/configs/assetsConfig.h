#pragma once
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#include "nameof.h"
#include "raylib.h"

namespace BreadEngine {
    struct File
    {
        File() = default;

        File(const std::string &fullName, const std::string &pathFromRoot, const std::string &extension, const std::string &guid)
        {
            this->_fullPath = fullName;
            this->_pathFromRoot = pathFromRoot;
            this->_extension = extension;
            this->_guid = guid;
        }

        ~File() = default;

        [[nodiscard]] const std::string &getGUID() const { return _guid; }
        [[nodiscard]] const std::string &getFullName() const { return _fullPath; }
        [[nodiscard]] const std::string &getPathFromRoot() const { return _pathFromRoot; }
        [[nodiscard]] const std::string &getExtension() const { return _extension; }

    private:
        friend struct YAML::convert<File>;
        std::string _guid;
        std::string _fullPath;
        std::string _pathFromRoot;
        std::string _extension;
    };

    struct Folder
    {
        Folder() = default;

        Folder(const int depth, const std::string &name, const std::string &guid)
        {
            this->_depth = depth;
            this->_name = name;
            this->_guid = guid;
        }

        ~Folder() = default;

        [[nodiscard]] bool isEmpty() const { return _files.empty() && _folders.empty(); }
        [[nodiscard]] int getDepth() const { return _depth; }
        [[nodiscard]] const std::string &getGUID() const { return _guid; }
        [[nodiscard]] const std::string &getName() const { return _name; }
        [[nodiscard]] std::vector<File> &getFiles() { return _files; }
        [[nodiscard]] std::vector<Folder> &getFolders() { return _folders; }

        bool operator==(const Folder &other) const
        {
            return _guid == other._guid && _depth == other._depth && _name == other._name && _files.data() == other._files.data() && _folders.data() == other._folders.data();
        }

    private:
        friend struct YAML::convert<Folder>;
        int _depth = 0;
        std::string _guid;
        std::string _name;
        std::vector<File> _files;
        std::vector<Folder> _folders;
    };

    class AssetsConfig
    {
    public:
        AssetsConfig() = default;

        ~AssetsConfig() = default;

        void serialize();

        void deserialize(const char *projectPath);

        void findAllAssets(const char *projectPath);

        [[nodiscard]] static bool isFolder(const char *path);

        [[nodiscard]] Folder *getRootFolder();

        [[nodiscard]] const std::string &getPathByGuid(const std::string &guid);

        [[nodiscard]] const std::string &getGuidByPath(const std::string &path);

        [[nodiscard]] static std::string getNewGUID();

        bool operator==(const AssetsConfig &other) const
        {
            return _projectPath == other._projectPath && _rootFolder == other._rootFolder;
        }

    private:
        friend struct YAML::convert<AssetsConfig>;
        std::string _projectPath;
        Folder _rootFolder = Folder(0, "Project Files", "0");

        void parseFolders(Folder &folder, const FilePathList &filePathList) noexcept;
    };
} // BreadEngine
