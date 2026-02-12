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
        const auto paths = LoadDirectoryFiles(path);
        const auto isFolder = paths.count > 0 && GetFileExtension(path) == nullptr;
        UnloadDirectoryFiles(paths);
        return isFolder;
    }

    Folder *FileSystem::getRootFolder()
    {
        return &_rootFolder;
    }

    void FileSystem::parseFolders(Folder &folder, const FilePathList &filePathList) noexcept
    {
        for (auto i = 0u; i < filePathList.count; i++)
        {
            const auto path = filePathList.paths[i];
            const auto isFolder = FileSystem::isFolder(path);
            const auto relativePathStart = path + strlen(_projectPath.c_str()) + 1;
            const auto fileName = strrchr(relativePathStart, '\\');
            auto pathFromRoot = fileName ? fileName + 1 : relativePathStart;
            if (isFolder)
            {
                auto subFolder = Folder(folder.getDepth() + 1, pathFromRoot);
                auto list = LoadDirectoryFiles(path);
                parseFolders(subFolder, list);
                folder.getFolders().push_back(std::move(subFolder));
                UnloadDirectoryFiles(list);
            }
            else
            {
                const auto ext = GetFileExtension(path);
                auto file = File(path, pathFromRoot, ext);
                folder.getFiles().push_back(std::move(file));
            }
        }
    }
} // BreadEngine
