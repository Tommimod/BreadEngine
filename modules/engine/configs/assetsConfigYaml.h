#pragma once
#include <yaml-cpp/node/node.h>

#include "asset.h"
#include "assetsConfig.h"
#include "meshAsset.h"
#include "textureAsset.h"
#include "nameof.h"

namespace YAML {
    template<>
    struct convert<BreadEngine::File>
    {
        static Node encode(const BreadEngine::File &rhs)
        {
            const auto fullPathName = NAMEOF(rhs._fullPath).c_str();
            const auto pathFromRootName = NAMEOF(rhs._pathFromRoot).c_str();
            const auto extensionName = NAMEOF(rhs._extension).c_str();
            const auto guidName = NAMEOF(rhs._guid).c_str();
            Node node;
            node[fullPathName] = rhs._fullPath;
            node[pathFromRootName] = rhs._pathFromRoot;
            node[extensionName] = rhs._extension;
            node[guidName] = rhs._guid;
            return node;
        }

        static bool decode(const Node &node, BreadEngine::File &rhs)
        {
            const auto fullPathName = NAMEOF(rhs._fullPath).c_str();
            const auto pathFromRootName = NAMEOF(rhs._pathFromRoot).c_str();
            const auto extensionName = NAMEOF(rhs._extension).c_str();
            const auto guidName = NAMEOF(rhs._guid).c_str();
            rhs._fullPath = node[fullPathName].as<std::string>();
            rhs._pathFromRoot = node[pathFromRootName].as<std::string>();
            rhs._extension = node[extensionName].as<std::string>();
            rhs._guid = node[guidName].as<std::string>();
            rhs._shortName = GetFileName(rhs._fullPath.c_str());
            return true;
        }
    };

    template<>
    struct convert<std::vector<BreadEngine::File> >
    {
        static Node encode(const std::vector<BreadEngine::File> &rhs)
        {
            Node node;
            for (const auto &rh: rhs)
            {
                node.push_back(rh);
            }
            return node;
        }

        static bool decode(const Node &node, std::vector<BreadEngine::File> &rhs)
        {
            for (auto i = 0u; i < node.size(); i++)
            {
                rhs.push_back(node[i].as<BreadEngine::File>());
            }
            return true;
        }
    };

    template<>
    struct convert<BreadEngine::Folder>
    {
        static Node encode(const BreadEngine::Folder &rhs)
        {
            Node node;
            const auto fullPathName = NAMEOF(rhs._fullPath).c_str();
            const auto depthName = NAMEOF(rhs._depth).c_str();
            const auto guidName = NAMEOF(rhs._guid).c_str();
            const auto pathFromRootName = NAMEOF(rhs._pathFromRoot).c_str();
            const auto shortName = NAMEOF(rhs._name).c_str();
            const auto filesName = NAMEOF(rhs._files).c_str();
            const auto foldersName = NAMEOF(rhs._folders).c_str();
            node[fullPathName] = rhs._fullPath;
            node[depthName] = rhs._depth;
            node[guidName] = rhs._guid;
            node[pathFromRootName] = rhs._pathFromRoot;
            node[shortName] = rhs._name;
            node[filesName] = rhs._files;
            node[foldersName] = rhs._folders;
            return node;
        }

        static bool decode(const Node &node, BreadEngine::Folder &rhs)
        {
            const auto fullPathName = NAMEOF(rhs._fullPath).c_str();
            const auto depthName = NAMEOF(rhs._depth).c_str();
            const auto guidName = NAMEOF(rhs._guid).c_str();
            const auto pathFromRootName = NAMEOF(rhs._pathFromRoot).c_str();
            const auto shortName = NAMEOF(rhs._name).c_str();
            const auto filesName = NAMEOF(rhs._files).c_str();
            const auto foldersName = NAMEOF(rhs._folders).c_str();
            rhs._fullPath = node[fullPathName].as<std::string>();
            rhs._depth = node[depthName].as<int>();
            rhs._guid = node[guidName].as<std::string>();
            rhs._pathFromRoot = node[pathFromRootName].as<std::string>();
            rhs._name = node[shortName].as<std::string>();
            rhs._files = node[filesName].as<std::vector<BreadEngine::File> >();
            rhs._folders = node[foldersName].as<std::vector<BreadEngine::Folder> >();
            return true;
        }
    };

    template<>
    struct convert<BreadEngine::AssetsConfig>
    {
        static Node encode(const BreadEngine::AssetsConfig &rhs)
        {
            constexpr static std::string typeKey = "type";
            const auto projectPathName = NAMEOF(rhs._projectPath).c_str();
            const auto folderName = NAMEOF(rhs._rootFolder).c_str();
            const auto guidToAssetName = NAMEOF(rhs._guidToAsset).c_str();
            Node node;
            node[projectPathName] = rhs._projectPath;
            node[folderName] = rhs._rootFolder;
            Node assetsNode(NodeType::Map);
            for (auto &[guid, value]: rhs._guidToAsset)
            {
                if (value == nullptr) continue;
                auto data = value->serialize();
                data[typeKey] = value->getTypeName();
                assetsNode[guid] = data;
            }
            node[guidToAssetName] = assetsNode;
            return node;
        }

        static bool decode(const Node &node, BreadEngine::AssetsConfig &rhs)
        {
            const auto projectPathName = NAMEOF(rhs._projectPath).c_str();
            const auto folderName = NAMEOF(rhs._rootFolder).c_str();
            rhs._projectPath = node[projectPathName].as<std::string>();
            rhs._rootFolder = node[folderName].as<BreadEngine::Folder>();
            return true;
        }
    };
}
