#pragma once
#include <string>
#include <vector>
#include "raylib.h"

namespace BreadEngine {
    struct Folder
    {
        int depth = 0;
        std::string name;
        std::vector<std::string> files;
        std::vector<Folder> folders;

        [[nodiscard]] bool isEmpty() const { return files.empty() && folders.empty(); }
    };

    class FileSystem
    {
    public:
        void initialize(const char *projectPath);

        [[nodiscard]] static bool isFolder(const char *path);

        [[nodiscard]] const Folder &getRootFolder();

    private:
        std::string _projectPath;
        Folder _rootFolder{.depth = 0, .name = "assets"};

        void parseFolders(Folder &folder, const FilePathList &filePathList);
    };
} // BreadEngine
