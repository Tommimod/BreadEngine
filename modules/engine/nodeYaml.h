#pragma once
#include <yaml-cpp/node/node.h>
#include "nameof.h"
#include "node.h"
#include "component/core/componentsProvider.h"
#include "inspectorObject.h"

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
            yamlNode["IsNodePrefab"] = !rhs.IsCopyPipeline;
            return yamlNode;
        }

        static bool decode(const Node &yamlNode, BreadEngine::NodeRawData &rhs)
        {
            unsigned int nodeId = INT_MAX;
            const auto idName = NAMEOF(BreadEngine::NodeRawData::ParentId).c_str();
            BreadEngine::Node *parentNode = nullptr;
            if (!yamlNode["IsNodePrefab"].as<bool>())
            {
                const auto id = yamlNode[idName].as<unsigned int>();
                if (id != INT_MAX)
                {
                    parentNode = BreadEngine::NodeProvider::getNode(id);
                }
            }

            // Node links (e.g. DeferredNodeLink) are resolved by node id, so a "real"
            // scene/prefab load (IsNodePrefab == true) must restore the exact ids that
            // were recorded at serialization time, otherwise references captured before
            // a full tree teardown (e.g. leaving playmode) can no longer be resolved.
            // Copy pipelines (IsNodePrefab == false, e.g. duplicate/paste) must NOT reuse
            // stored ids, since the source nodes they were copied from are still alive.
            const auto preserveIds = yamlNode["IsNodePrefab"].as<bool>();

            BreadEngine::InspectorStruct::beginDeserializationPhase();
            decodeRecursive(yamlNode, nodeId, parentNode, preserveIds);

            BreadEngine::InspectorStruct::resolveAllDeferredNodeLinks();
            BreadEngine::InspectorStruct::resolveAllDeferredAssetLinks();
            rhs.Id = nodeId;
            BreadEngine::InspectorStruct::endDeserializationPhase();
            return true;
        }

        static void decodeRecursive(const Node &yamlNode, unsigned int &nodeId, BreadEngine::Node *parentNode, bool preserveIds)
        {
            const auto isActiveName = NAMEOF(BreadEngine::NodeRawData::IsActive).c_str();
            const auto nameName = NAMEOF(BreadEngine::NodeRawData::Name).c_str();
            const auto idName = NAMEOF(BreadEngine::NodeRawData::Id).c_str();
            constexpr auto componentsName = "Components";
            constexpr auto childsName = "Childs";

            auto name = yamlNode[nameName].as<std::string>();
            const auto savedId = yamlNode[idName].as<unsigned int>();
            const auto isRoot = parentNode == nullptr && yamlNode["IsNodePrefab"].as<bool>();

            BreadEngine::Node *nextNodePtr;
            if (isRoot)
            {
                nextNodePtr = &BreadEngine::Engine::getRootNode();
                if (preserveIds)
                {
                    BreadEngine::NodeProvider::assignId(*nextNodePtr, savedId);
                }
            }
            else
            {
                auto &newNode = preserveIds
                                    ? BreadEngine::NodeProvider::createNode(savedId)
                                    : BreadEngine::NodeProvider::createNode();
                nextNodePtr = parentNode != nullptr ? &newNode.setup(name, *parentNode) : &newNode.setup(name);
            }

            auto &nextNode = *nextNodePtr;
            if (nextNode._name.empty()) nextNode.setup(name);
            nextNode.setIsActive(yamlNode[isActiveName].as<bool>());
            auto componentsNode = yamlNode[componentsName].as<Node>();
            for (unsigned i = 0; i < componentsNode.size(); i++)
            {
                constexpr auto componentTypeName = "ComponentType";
                auto compNode = componentsNode[i];
                auto compType = compNode[componentTypeName].as<std::string>();
                BreadEngine::ComponentsProvider::addDynamic(nextNode.getId(), compType, compNode);
            }

            for (auto childsNodeList = yamlNode[childsName].as<std::vector<Node> >(); auto &childNode: childsNodeList)
            {
                decodeRecursive(childNode, nodeId, &nextNode, preserveIds);
            }

            nodeId = nextNode.getId();
        }
    };
}
