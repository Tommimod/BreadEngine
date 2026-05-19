#include "inspectorObject.h"
#include <cxxabi.h>

#include "engine.h"
#include "node.h"
#include "core/component.h"
#include "component/core/componentsProvider.h"
#include "configs/asset.h"
#include "tracy/Tracy.hpp"
#include "raylib.h"

namespace BreadEngine {
    std::string InspectorStruct::getTypeName()
    {
        ZoneScoped;
        int status;
        auto *mangled = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, &status);
        std::string result = status == 0 ? mangled : typeid(*this).name();
        std::free(mangled);

        if (const size_t lastColon = result.rfind("::"); lastColon != std::string::npos)
        {
            return result.substr(lastColon + 2);
        }
        return result;
    }

    std::string InspectorStruct::getNewGUID()
    {
        ZoneScoped;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);
        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++)
        {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++)
        {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++)
        {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++)
        {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++)
        {
            ss << dis(gen);
        };
        return ss.str();
    }

    YAML::Node InspectorStruct::serialize()
    {
        ZoneScoped;
        YAML::Node node(YAML::NodeType::Map);
        for (const auto &prop: getInspectedProperties())
        {
            auto val = prop.get(this);
            node[prop.name] = propertyToYaml(prop, val);
        }
        return node;
    }

    void InspectorStruct::deserialize(const YAML::Node &node)
    {
        deserialize(node, "");
    }

    void InspectorStruct::deserialize(const YAML::Node &node, const std::string &parentPath)
    {
        ZoneScoped;
        for (const auto &prop: getInspectedProperties())
        {
            if (node[prop.name])
            {
                const auto &subnode = node[prop.name];
                std::string fullPath = parentPath.empty() ? prop.name : parentPath + "." + prop.name;

                if (prop.type == PropertyType::INSPECTOR_STRUCT)
                {
                    auto val = prop.get(this);
                    auto *strct = std::any_cast<InspectorStruct *>(val);
                    if (strct)
                    {
                        strct->deserialize(subnode, fullPath);
                    }
                }
                else if (prop.type == PropertyType::VECTOR_L)
                {
                    auto val = prop.get(this);
                    auto acc = std::any_cast<std::shared_ptr<VectorAccessor> >(val);
                    while (acc->size() > 0) acc->remove(0);
                    if (subnode.IsSequence())
                    {
                        auto elType = acc->elementType();
                        if (elType == PropertyType::VECTOR_L)
                        {
                            throw std::runtime_error("Nested vectors not supported for deserialization");
                        }
                        for (size_t i = 0; i < subnode.size(); ++i)
                        {
                            const auto &elemn = subnode[i];
                            acc->add();
                            if (elType == PropertyType::INSPECTOR_STRUCT)
                            {
                                auto lastval = acc->get(acc->size() - 1);
                                auto *strct = std::any_cast<InspectorStruct *>(lastval);
                                if (strct)
                                {
                                    std::string elementPath = fullPath + "[" + std::to_string(i) + "]";
                                    strct->deserialize(elemn, elementPath);
                                }
                            }
                            else
                            {
                                Property::VariantT subval = yamlToVariant(elType, elemn, fullPath);
                                acc->set(acc->size() - 1, subval);
                            }
                        }
                    }
                }
                else
                {
                    Property::VariantT val = yamlToVariant(prop.type, subnode, fullPath);
                    prop.set(this, val);
                }
            }
        }
    }

    YAML::Node InspectorStruct::propertyToYaml(const Property &prop, const Property::VariantT &val)
    {
        ZoneScoped;
        switch (prop.type)
        {
            case PropertyType::INT: return YAML::Node(std::any_cast<int>(val));
            case PropertyType::FLOAT: return YAML::Node(std::any_cast<float>(val));
            case PropertyType::LONG: return YAML::Node(std::any_cast<long>(val));
            case PropertyType::BOOL: return YAML::Node(std::any_cast<bool>(val));
            case PropertyType::STRING: return YAML::Node(std::any_cast<std::string>(val));
            case PropertyType::VECTOR2:
            {
                auto v = std::any_cast<Vector2>(val);
                YAML::Node n;
                n["x"] = v.x;
                n["y"] = v.y;
                return n;
            }
            case PropertyType::VECTOR3:
            {
                auto v = std::any_cast<Vector3>(val);
                YAML::Node n;
                n["x"] = v.x;
                n["y"] = v.y;
                n["z"] = v.z;
                return n;
            }
            case PropertyType::VECTOR4:
            {
                auto v = std::any_cast<Vector4>(val);
                YAML::Node n;
                n["x"] = v.x;
                n["y"] = v.y;
                n["z"] = v.z;
                n["w"] = v.w;
                return n;
            }
            case PropertyType::COLOR:
            {
                auto c = std::any_cast<Color>(val);
                YAML::Node n;
                n["r"] = static_cast<int>(c.r);
                n["g"] = static_cast<int>(c.g);
                n["b"] = static_cast<int>(c.b);
                n["a"] = static_cast<int>(c.a);
                return n;
            }
            case PropertyType::ENUM: return YAML::Node(std::any_cast<int>(val));
            case PropertyType::INSPECTOR_STRUCT:
            {
                auto *strct = std::any_cast<InspectorStruct *>(val);
                if (strct)
                {
                    return strct->serialize();
                }
                return YAML::Node(YAML::NodeType::Null);
            }
            case PropertyType::NODE_LINK:
            {
                auto *comp = std::any_cast<Component *>(val);
                if (comp && comp->getOwner())
                {
                    YAML::Node n;
                    n["nodeId"] = comp->getOwner()->getId();
                    n["type"] = comp->getTypeName();
                    return n;
                }

                return YAML::Node(0);
            }
            case PropertyType::ASSET_LINK:
            {
                auto *asset = std::any_cast<Asset *>(val);
                if (asset)
                {
                    YAML::Node n;
                    n["guid"] = asset->getGuid().c_str();
                    n["type"] = asset->getTypeName();
                    return n;
                }

                return YAML::Node(YAML::NodeType::Null);
            }
            case PropertyType::VECTOR_L:
            {
                auto acc = std::any_cast<std::shared_ptr<VectorAccessor> >(val);
                if (acc->elementType() == PropertyType::VECTOR_L)
                {
                    throw std::runtime_error("Nested vectors not supported for serialization");
                }
                YAML::Node seq(YAML::NodeType::Sequence);
                for (size_t i = 0; i < acc->size(); ++i)
                {
                    auto subval = acc->get(i);
                    switch (auto elType = acc->elementType(); elType)
                    {
                        case PropertyType::INT: seq.push_back(std::any_cast<int>(subval));
                            break;
                        case PropertyType::FLOAT: seq.push_back(std::any_cast<float>(subval));
                            break;
                        case PropertyType::LONG: seq.push_back(std::any_cast<long>(subval));
                            break;
                        case PropertyType::BOOL: seq.push_back(std::any_cast<bool>(subval));
                            break;
                        case PropertyType::STRING: seq.push_back(std::any_cast<std::string>(subval));
                            break;
                        case PropertyType::VECTOR2:
                        {
                            auto v = std::any_cast<Vector2>(subval);
                            YAML::Node nn;
                            nn["x"] = v.x;
                            nn["y"] = v.y;
                            seq.push_back(nn);
                            break;
                        }
                        case PropertyType::VECTOR3:
                        {
                            auto v = std::any_cast<Vector3>(subval);
                            YAML::Node nn;
                            nn["x"] = v.x;
                            nn["y"] = v.y;
                            nn["z"] = v.z;
                            seq.push_back(nn);
                            break;
                        }
                        case PropertyType::VECTOR4:
                        {
                            auto v = std::any_cast<Vector4>(subval);
                            YAML::Node nn;
                            nn["x"] = v.x;
                            nn["y"] = v.y;
                            nn["z"] = v.z;
                            nn["w"] = v.w;
                            seq.push_back(nn);
                            break;
                        }
                        case PropertyType::COLOR:
                        {
                            auto c = std::any_cast<Color>(subval);
                            YAML::Node nn;
                            nn["r"] = static_cast<int>(c.r);
                            nn["g"] = static_cast<int>(c.g);
                            nn["b"] = static_cast<int>(c.b);
                            nn["a"] = static_cast<int>(c.a);
                            seq.push_back(nn);
                            break;
                        }
                        case PropertyType::ENUM: seq.push_back(std::any_cast<int>(subval));
                            break;
                        case PropertyType::INSPECTOR_STRUCT:
                        {
                            auto *strct = std::any_cast<InspectorStruct *>(subval);
                            if (strct)
                            {
                                seq.push_back(strct->serialize());
                            }
                            else
                            {
                                seq.push_back(YAML::Node(YAML::NodeType::Null));
                            }
                            break;
                        }
                        default: throw std::runtime_error("Unsupported element type in vector for serialization");
                    }
                }
                return seq;
            }
            default: throw std::runtime_error("Unsupported property type for serialization");
        }
    }

    Property::VariantT InspectorStruct::yamlToVariant(const PropertyType type, const YAML::Node &node)
    {
        return yamlToVariant(type, node, "");
    }

    Property::VariantT InspectorStruct::yamlToVariant(const PropertyType type, const YAML::Node &n, const std::string &propertyName)
    {
        ZoneScoped;
        switch (type)
        {
            case PropertyType::INT: return Property::VariantT{n.as<int>()};
            case PropertyType::FLOAT: return Property::VariantT{n.as<float>()};
            case PropertyType::LONG: return Property::VariantT{n.as<long>()};
            case PropertyType::BOOL: return Property::VariantT{n.as<bool>()};
            case PropertyType::STRING: return Property::VariantT{n.as<std::string>()};
            case PropertyType::VECTOR2:
            {
                Vector2 v;
                v.x = n["x"].as<float>();
                v.y = n["y"].as<float>();
                return Property::VariantT{v};
            }
            case PropertyType::VECTOR3:
            {
                Vector3 v;
                v.x = n["x"].as<float>();
                v.y = n["y"].as<float>();
                v.z = n["z"].as<float>();
                return Property::VariantT{v};
            }
            case PropertyType::VECTOR4:
            {
                Vector4 v;
                v.x = n["x"].as<float>();
                v.y = n["y"].as<float>();
                v.z = n["z"].as<float>();
                v.w = n["w"].as<float>();
                return Property::VariantT{v};
            }
            case PropertyType::COLOR:
            {
                Color c;
                c.r = static_cast<unsigned char>(n["r"].as<int>());
                c.g = static_cast<unsigned char>(n["g"].as<int>());
                c.b = static_cast<unsigned char>(n["b"].as<int>());
                c.a = static_cast<unsigned char>(n["a"].as<int>());
                return Property::VariantT{c};
            }
            case PropertyType::ENUM: return Property::VariantT{n.as<int>()};
            case PropertyType::NODE_LINK:
            {
                if (n.size() == 0) return Property::VariantT{static_cast<Component *>(nullptr)};

                const auto targetNodeId = n["nodeId"].as<unsigned int>();
                const auto targetType = n["type"].as<std::string>();
                if (isInDeserializationPhase())
                {
                    if (Component *currentComp = getCurrentDeserializingComponent())
                    {
                        DeferredNodeLink deferred;
                        unsigned int ownerId = currentComp->getOwner()
                                                   ? currentComp->getOwner()->getId()
                                                   : getCurrentDeserializingOwnerId();
                        deferred.sourceOwnerId = ownerId;
                        deferred.sourceComponentType = currentComp->getTypeName();
                        deferred.propertyPath = propertyName;
                        deferred.targetNodeId = targetNodeId;
                        deferred.targetComponentType = targetType;
                        getDeferredNodeLinks().push_back(std::move(deferred));
                    }
                    return Property::VariantT{static_cast<Component *>(nullptr)};
                }

                const auto allComps = ComponentsProvider::getAllComponents(targetNodeId);
                Component *comp = nullptr;
                for (const auto c: allComps)
                {
                    if (c->getTypeName() == targetType)
                    {
                        comp = c;
                        break;
                    }
                }
                return Property::VariantT{comp};
            }
            case PropertyType::ASSET_LINK:
            {
                if (!n["guid"] || !n["type"]) return Property::VariantT{static_cast<Asset *>(nullptr)};

                const auto targetGuid = n["guid"].as<std::string>();
                const auto targetType = n["type"].as<std::string>();

                if (isInDeserializationPhase())
                {
                    if (Component *currentComp = getCurrentDeserializingComponent())
                    {
                        DeferredAssetLink deferred;
                        unsigned int ownerId = currentComp->getOwner()
                                                   ? currentComp->getOwner()->getId()
                                                   : getCurrentDeserializingOwnerId();
                        deferred.sourceOwnerId = ownerId;
                        deferred.sourceComponentType = currentComp->getTypeName();
                        deferred.propertyPath = propertyName;
                        deferred.targetAssetGuid = targetGuid;
                        deferred.targetAssetType = targetType;
                        getDeferredAssetLinks().push_back(std::move(deferred));
                    }
                    return Property::VariantT{static_cast<Asset *>(nullptr)};
                }

                auto &assetsConfig = Engine::getInstance().getAssetsConfig();
                auto file = assetsConfig.getFileByGuid(targetGuid);
                if (!file)
                {
                    TraceLog(LOG_ERROR, TextFormat("Failed to find asset by guid: %s", targetGuid.c_str()));
                    return Property::VariantT{static_cast<Asset *>(nullptr)};
                }

                auto asset = assetsConfig.getAsset(file);
                if (asset)
                {
                    return Property::VariantT{asset.get()};
                }
                return Property::VariantT{static_cast<Asset *>(nullptr)};
            }
            default: throw std::runtime_error("Unsupported property type for deserialization");
        }
    }

    std::vector<DeferredNodeLink> &InspectorStruct::getDeferredNodeLinks()
    {
        static std::vector<DeferredNodeLink> deferredLinks;
        return deferredLinks;
    }

    std::vector<DeferredAssetLink> &InspectorStruct::getDeferredAssetLinks()
    {
        static std::vector<DeferredAssetLink> deferredAssetLinks;
        return deferredAssetLinks;
    }

    bool &InspectorStruct::getDeserializationFlag()
    {
        static bool inDeserialization = false;
        return inDeserialization;
    }

    void InspectorStruct::beginDeserializationPhase()
    {
        getDeferredNodeLinks().clear();
        getDeferredAssetLinks().clear();
        getDeserializationFlag() = true;
    }

    void InspectorStruct::endDeserializationPhase()
    {
        getDeserializationFlag() = false;
    }

    bool InspectorStruct::isInDeserializationPhase()
    {
        return getDeserializationFlag();
    }

    bool InspectorStruct::setPropertyByPath(InspectorStruct *obj, const std::string &path, const Property::VariantT &value, PropertyType expectedType)
    {
        ZoneScoped;
        if (!obj || path.empty()) return false;

        std::string remainingPath = path;
        InspectorStruct *currentObj = obj;

        while (true)
        {
            size_t dotPos = remainingPath.find('.');
            size_t bracketPos = remainingPath.find('[');

            std::string currentSegment;
            std::string nextPath;

            if (bracketPos != std::string::npos && (dotPos == std::string::npos || bracketPos < dotPos))
            {
                currentSegment = remainingPath.substr(0, bracketPos);
                size_t closeBracket = remainingPath.find(']', bracketPos);
                if (closeBracket == std::string::npos) return false;

                std::string indexStr = remainingPath.substr(bracketPos + 1, closeBracket - bracketPos - 1);
                int index = std::stoi(indexStr);

                size_t nextDot = remainingPath.find('.', closeBracket);
                if (nextDot != std::string::npos)
                {
                    nextPath = remainingPath.substr(nextDot + 1);
                }
                else
                {
                    nextPath = "";
                }

                for (auto &prop: currentObj->getInspectedProperties())
                {
                    if (prop.name == currentSegment)
                    {
                        if (prop.type == PropertyType::VECTOR_L)
                        {
                            auto acc = std::any_cast<std::shared_ptr<VectorAccessor> >(prop.get(currentObj));
                            if (index >= 0 && index < static_cast<int>(acc->size()))
                            {
                                if (nextPath.empty())
                                {
                                    Property::VariantT val = value;
                                    acc->set(index, val);
                                    return true;
                                }
                                else
                                {
                                    auto elemVal = acc->get(index);
                                    auto *strct = std::any_cast<InspectorStruct *>(elemVal);
                                    if (strct)
                                    {
                                        currentObj = strct;
                                        remainingPath = nextPath;
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }

                if (nextPath.empty()) return false;
            }
            else if (dotPos != std::string::npos)
            {
                currentSegment = remainingPath.substr(0, dotPos);
                nextPath = remainingPath.substr(dotPos + 1);

                bool found = false;
                for (auto &prop: currentObj->getInspectedProperties())
                {
                    if (prop.name == currentSegment)
                    {
                        if (prop.type == PropertyType::INSPECTOR_STRUCT)
                        {
                            auto val = prop.get(currentObj);
                            auto *strct = std::any_cast<InspectorStruct *>(val);
                            if (strct)
                            {
                                currentObj = strct;
                                remainingPath = nextPath;
                                found = true;
                                break;
                            }
                        }
                        else if (prop.type == PropertyType::VECTOR_L)
                        {
                            auto acc = std::any_cast<std::shared_ptr<VectorAccessor> >(prop.get(currentObj));
                            for (size_t i = 0; i < acc->size(); ++i)
                            {
                                auto elemVal = acc->get(i);
                                auto *strct = std::any_cast<InspectorStruct *>(elemVal);
                                if (strct && setPropertyByPath(strct, nextPath, value, expectedType))
                                {
                                    return true;
                                }
                            }
                            return false;
                        }
                    }
                }

                if (!found) return false;
            }
            else
            {
                currentSegment = remainingPath;
                for (auto &prop: currentObj->getInspectedProperties())
                {
                    if (prop.name == currentSegment && prop.type == expectedType)
                    {
                        prop.set(currentObj, value);
                        return true;
                    }
                }
                return false;
            }
        }
    }

    void InspectorStruct::resolveAllDeferredNodeLinks()
    {
        ZoneScoped;
        for (const auto &link: getDeferredNodeLinks())
        {
            const auto allComps = ComponentsProvider::getAllComponents(link.targetNodeId);
            Component *targetComp = nullptr;
            for (const auto c: allComps)
            {
                if (c->getTypeName() == link.targetComponentType)
                {
                    targetComp = c;
                    break;
                }
            }

            if (!targetComp)
            {
                Logger::LogWarning(TextFormat("Could not resolve node link: target component %s not found in node %u",
                                              link.targetComponentType.c_str(), link.targetNodeId));
                continue;
            }

            const auto sourceComps = ComponentsProvider::getAllComponents(link.sourceOwnerId);
            Component *sourceComp = nullptr;
            for (const auto c: sourceComps)
            {
                if (c->getTypeName() == link.sourceComponentType)
                {
                    sourceComp = c;
                    break;
                }
            }

            if (!sourceComp)
            {
                Logger::LogWarning(TextFormat("Could not resolve node link: source component %s not found in node %u",
                                              link.sourceComponentType.c_str(), link.sourceOwnerId));
                continue;
            }

            if (!setPropertyByPath(sourceComp, link.propertyPath, Property::VariantT{targetComp}, PropertyType::NODE_LINK))
            {
                Logger::LogWarning(TextFormat("Could not resolve node link: property path %s not found in component %s",
                                              link.propertyPath.c_str(), link.sourceComponentType.c_str()));
            }
        }

        getDeferredNodeLinks().clear();
        getDeserializationFlag() = false;
    }

    void InspectorStruct::resolveAllDeferredAssetLinks()
    {
        ZoneScoped;
        auto &assetsConfig = Engine::getInstance().getAssetsConfig();

        for (const auto &link: getDeferredAssetLinks())
        {
            auto file = assetsConfig.getFileByGuid(link.targetAssetGuid);
            if (!file)
            {
                Logger::LogWarning(TextFormat("Could not resolve asset link: asset with guid %s not found",
                                              link.targetAssetGuid.c_str()));
                continue;
            }

            auto targetAsset = assetsConfig.getAsset(file);
            if (!targetAsset)
            {
                Logger::LogWarning(TextFormat("Could not resolve asset link: asset %s (%s) could not be loaded",
                                              link.targetAssetType.c_str(), link.targetAssetGuid.c_str()));
                continue;
            }

            const auto sourceComps = ComponentsProvider::getAllComponents(link.sourceOwnerId);
            Component *sourceComp = nullptr;
            for (const auto c: sourceComps)
            {
                if (c->getTypeName() == link.sourceComponentType)
                {
                    sourceComp = c;
                    break;
                }
            }

            if (!sourceComp)
            {
                Logger::LogWarning(TextFormat("Could not resolve asset link: source component %s not found in node %u",
                                              link.sourceComponentType.c_str(), link.sourceOwnerId));
                continue;
            }

            if (!setPropertyByPath(sourceComp, link.propertyPath, Property::VariantT{targetAsset.get()}, PropertyType::ASSET_LINK))
            {
                Logger::LogWarning(TextFormat("Could not resolve asset link: property path %s not found in component %s",
                                              link.propertyPath.c_str(), link.sourceComponentType.c_str()));
            }
        }

        getDeferredAssetLinks().clear();
    }

    Component *&InspectorStruct::getCurrentComponent()
    {
        static Component *current = nullptr;
        return current;
    }

    void InspectorStruct::setCurrentDeserializingComponent(Component *comp)
    {
        getCurrentComponent() = comp;
    }

    Component *InspectorStruct::getCurrentDeserializingComponent()
    {
        return getCurrentComponent();
    }

    unsigned int &InspectorStruct::getCurrentOwnerId()
    {
        static unsigned int ownerId = 0;
        return ownerId;
    }

    void InspectorStruct::setCurrentDeserializingOwnerId(const unsigned int ownerId)
    {
        getCurrentOwnerId() = ownerId;
    }

    unsigned int InspectorStruct::getCurrentDeserializingOwnerId()
    {
        return getCurrentOwnerId();
    }
} // BreadEngine
