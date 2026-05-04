#pragma once
#include "file.h"
#include "inspectorObject.h"
#include <vector>
#include <string>
#include <yaml-cpp/node/node.h>

namespace BreadEngine {
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
        [[nodiscard]] const std::string &getFullPath() const { return _fullPath; }
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
        std::vector<Folder> _folders;
        std::vector<File> _files;
        std::string _fullPath;
        std::string _guid;
        std::string _pathFromRoot;
        std::string _name;
        int _depth = 0;

        INSPECTOR_BEGIN(Folder)

        INSPECT_FIELD (_depth);
        INSPECT_FIELD (_guid);
        INSPECT_FIELD (_pathFromRoot);
        INSPECT_FIELD (_files);
        INSPECT_FIELD (_folders);

        INSPECTOR_END()
    };
} // BreadEngine
