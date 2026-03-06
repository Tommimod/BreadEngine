#pragma once
#include <yaml-cpp/node/node.h>
#include "nameof.h"
#include "node.h"

namespace YAML {
    template<>
    struct convert<BreadEngine::NodeRawData>
    {
        static Node encode(const BreadEngine::NodeRawData &rhs)
        {
            const auto isActiveName = NAMEOF(rhs.IsActive).c_str();
            const auto idName = NAMEOF(rhs.Id).c_str();
            const auto nameName = NAMEOF(rhs.Name).c_str();
            const auto parentIdName = NAMEOF(rhs.ParentId).c_str();
            const auto childsIdName = NAMEOF(rhs.ChildsIds).c_str();
            Node yamlNode;
            yamlNode[isActiveName] = rhs.IsActive;
            yamlNode[idName] = rhs.Id;
            yamlNode[nameName] = rhs.Name;
            yamlNode[parentIdName] = rhs.ParentId;
            std::vector<unsigned int> childsIds;
            for (const auto &child: rhs.ChildsIds)
            {
                childsIds.emplace_back(child);
            }
            yamlNode[childsIdName] = childsIds;
            return yamlNode;
        }

        static bool decode(const Node &yamlNode, BreadEngine::NodeRawData &rhs)
        {
            const auto isActiveName = NAMEOF(rhs.IsActive).c_str();
            const auto idName = NAMEOF(rhs.Id).c_str();
            const auto nameName = NAMEOF(rhs.Name).c_str();
            const auto parentIdName = NAMEOF_FULL(rhs.ParentId).c_str();
            const auto childsIdName = NAMEOF(rhs.ChildsIds).c_str();
            rhs.IsActive = yamlNode[isActiveName].as<bool>();
            rhs.Id = yamlNode[idName].as<unsigned int>();
            rhs.ParentId = yamlNode[parentIdName].as<unsigned int>();
            rhs.ChildsIds = yamlNode[childsIdName].as<std::vector<unsigned int>>();
            rhs.Name = yamlNode[nameName].as<std::string>();
            return true;
        }
    };
}
