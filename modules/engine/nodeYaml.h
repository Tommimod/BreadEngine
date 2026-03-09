#pragma once
#include <yaml-cpp/node/node.h>
#include "nameof.h"
#include "node.h"
#include "component/componentsProvider.h"

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
            Node compNode;
            const auto components = BreadEngine::ComponentsProvider::getAllComponents(rhs.Id);
            for (BreadEngine::Component *component: components)
            {
                compNode.push_back(component->serialize());
            }

            Node yamlNode;
            yamlNode[isActiveName] = rhs.IsActive;
            yamlNode[idName] = rhs.Id;
            yamlNode[nameName] = rhs.Name;
            yamlNode[parentIdName] = rhs.ParentId;
            yamlNode["Components"] = compNode;
            std::vector<Node> childNodes;
            for (const auto &childId: rhs.ChildsIds)
            {
                auto currentChild = BreadEngine::NodeProvider::getNode(childId);
                std::vector<unsigned int> childIds;
                childIds.reserve(currentChild->getChildCount());
                for (auto child: currentChild->getAllChilds())
                {
                    childIds.emplace_back(child->getId());
                }
                auto rawData = BreadEngine::NodeRawData
                {
                    .ChildsIds = std::move(childIds),
                    .ParentId = currentChild->getParent()->getId(),
                    .Name = currentChild->getName(),
                    .Id = childId,
                    .IsActive = currentChild->getIsActive(),
                };
                childNodes.emplace_back(encode(rawData));
            }
            yamlNode["Childs"] = childNodes;
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
            rhs.ChildsIds = yamlNode[childsIdName].as<std::vector<unsigned int> >();
            rhs.Name = yamlNode[nameName].as<std::string>();
            return true;
        }
    };
}
