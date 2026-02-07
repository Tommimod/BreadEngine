#include "fileSystem.h"
#include "raylib.h"

namespace BreadEngine {
    void FileSystem::initialize(const char *projectPath)
    {
        _projectPath = projectPath;
        const FilePathList filesProvider = LoadDirectoryFiles(projectPath);
        parseFolders(_rootFolder, filesProvider);
        UnloadDirectoryFiles(filesProvider);
    }

    bool FileSystem::isFolder(const char *path)
    {
        return LoadDirectoryFiles(path).count > 0 && GetFileExtension(path) == nullptr;
    }

    const Folder &FileSystem::getRootFolder()
    {
        return _rootFolder;
    }

    void FileSystem::parseFolders(Folder &folder, const FilePathList &filePathList)
    {
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = FileSystem::isFolder(path);
            int cnt = 0;
            std::string pathFromRoot = *TextSplit(path + strlen(_projectPath.c_str()) + 1, '\\', &cnt);
            if (isFolder)
            {
                Folder subFolder{.depth = folder.depth + 1, .name = std::move(pathFromRoot)};
                auto list = LoadDirectoryFiles(path);
                parseFolders(subFolder, list);
                folder.folders.push_back(std::move(subFolder));
                UnloadDirectoryFiles(list);
            }
            else
            {
                folder.files.push_back(std::move(pathFromRoot));
            }
        }
    }
} // BreadEngine
