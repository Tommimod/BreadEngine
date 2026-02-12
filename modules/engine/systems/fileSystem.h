#pragma once
#include <string>
#include <vector>
#include "raylib.h"

namespace BreadEngine {
    struct File
    {
        File(const std::string &fullName, const std::string &pathFromRoot, const std::string &extension)
        {
            this->_fullPath = fullName;
            this->_pathFromRoot = pathFromRoot;
            this->_extension = extension;
        }

        [[nodiscard]] const std::string &getFullName() const { return _fullPath; }
        [[nodiscard]] const std::string &getPathFromRoot() const { return _pathFromRoot; }
        [[nodiscard]] const std::string &getExtension() const { return _extension; }

    private:
        std::string _fullPath;
        std::string _pathFromRoot;
        std::string _extension;
    };

    struct Folder
    {
        Folder(const int depth, const std::string &name)
        {
            this->_depth = depth;
            this->_name = name;
        }

        [[nodiscard]] bool isEmpty() const { return _files.empty() && _folders.empty(); }
        [[nodiscard]] int getDepth() const { return _depth; }
        [[nodiscard]] const std::string &getName() const { return _name; }
        [[nodiscard]] std::vector<File> &getFiles() { return _files; }
        [[nodiscard]] std::vector<Folder> &getFolders() { return _folders; }

        bool operator==(const Folder &other) const
        {
            return _depth == other._depth && _name == other._name && _files.data() == other._files.data() && _folders.data() == other._folders.data();
        }

    private:
        int _depth = 0;
        std::string _name;
        std::vector<File> _files;
        std::vector<Folder> _folders;
    };

    class FileSystem
    {
    public:
        void initialize(const char *projectPath);

        [[nodiscard]] static bool isFolder(const char *path);

        [[nodiscard]] Folder *getRootFolder();

    private:
        std::string _projectPath;
        Folder _rootFolder = Folder(0, "Project Files");

        void parseFolders(Folder &folder, const FilePathList &filePathList) noexcept;
    };
} // BreadEngine
