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
            constexpr auto componentsName = "Components";
            constexpr auto childsName = "Childs";
            Node compNode(NodeType::Sequence);
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
            yamlNode[componentsName] = compNode;
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
            yamlNode[childsName] = childNodes;
            return yamlNode;
        }

        static bool decode(const Node &yamlNode, BreadEngine::NodeRawData &rhs)
        {
            unsigned int nodeId = INT_MAX;
            decodeRecursive(yamlNode, nodeId, nullptr);
            rhs.Id = nodeId;
            return true;
        }

        static void decodeRecursive(const Node &yamlNode, unsigned int &nodeId, BreadEngine::Node *parentNode)
        {
            const auto isActiveName = NAMEOF(BreadEngine::NodeRawData::IsActive).c_str();
            const auto idName = NAMEOF(BreadEngine::NodeRawData::Id).c_str();
            const auto nameName = NAMEOF(BreadEngine::NodeRawData::Name).c_str();
            constexpr auto componentsName = "Components";
            constexpr auto childsName = "Childs";
            auto id = yamlNode[idName].as<unsigned int>();
            if (parentNode == nullptr)
            {
                nodeId = id;
            }

            auto name = yamlNode[nameName].as<std::string>();
            auto &nextNode = BreadEngine::Engine::nodePool.get();
            nextNode = parentNode == nullptr ? nextNode.setupAsRoot(name) : nextNode.setup(name, *parentNode);
            nextNode.setIsActive(yamlNode[isActiveName].as<bool>());
            auto componentsNode = yamlNode[componentsName].as<Node>();
            for (unsigned i = 0; i < componentsNode.size(); i++)
            {
                constexpr auto componentTypeName = "ComponentType";
                auto compNode = componentsNode[i];
                auto compType = compNode[componentTypeName].as<std::string>();
                BreadEngine::ComponentsProvider::addDynamic(nextNode.getId(), compType, compNode);
            }

            auto childsNodeList = yamlNode[childsName].as<std::vector<Node> >();
            for (auto &childNode: childsNodeList)
            {
                decodeRecursive(childNode, nodeId, &nextNode);
            }
        }
    };
}
