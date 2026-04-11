#include "inspectorObject.h"
#include <cxxabi.h>

#include "tracy/Tracy.hpp"

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
        ZoneScoped;
        for (const auto &prop: getInspectedProperties())
        {
            if (node[prop.name])
            {
                const auto &subnode = node[prop.name];
                if (prop.type == PropertyType::INSPECTOR_STRUCT)
                {
                    auto val = prop.get(this);
                    auto *strct = std::any_cast<InspectorStruct *>(val);
                    strct->deserialize(subnode);
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
                        for (const auto &elemn: subnode)
                        {
                            acc->add();
                            if (elType == PropertyType::INSPECTOR_STRUCT)
                            {
                                auto lastval = acc->get(acc->size() - 1);
                                auto *strct = std::any_cast<InspectorStruct *>(lastval);
                                strct->deserialize(elemn);
                            }
                            else
                            {
                                Property::VariantT subval = yamlToVariant(elType, elemn);
                                acc->set(acc->size() - 1, subval);
                            }
                        }
                    }
                }
                else
                {
                    Property::VariantT val = yamlToVariant(prop.type, subnode);
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
                n["r"] = c.r;
                n["g"] = c.g;
                n["b"] = c.b;
                n["a"] = c.a;
                return n;
            }
            case PropertyType::ENUM: return YAML::Node(static_cast<int>(std::any_cast<uint8_t>(val)));
            case PropertyType::INSPECTOR_STRUCT:
            {
                auto *strct = std::any_cast<InspectorStruct *>(val);
                return strct->serialize();
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
                            nn["r"] = c.r;
                            nn["g"] = c.g;
                            nn["b"] = c.b;
                            nn["a"] = c.a;
                            seq.push_back(nn);
                            break;
                        }
                        case PropertyType::ENUM: seq.push_back(static_cast<int>(std::any_cast<uint8_t>(subval)));
                            break;
                        case PropertyType::INSPECTOR_STRUCT: seq.push_back(std::any_cast<InspectorStruct *>(subval)->serialize());
                            break;
                        default: throw std::runtime_error("Unsupported element type in vector for serialization");
                    }
                }
                return seq;
            }
            default: throw std::runtime_error("Unsupported property type for serialization");
        }
    }

    Property::VariantT InspectorStruct::yamlToVariant(const PropertyType type, const YAML::Node &n)
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
                c.r = n["r"].as<unsigned char>();
                c.g = n["g"].as<unsigned char>();
                c.b = n["b"].as<unsigned char>();
                c.a = n["a"].as<unsigned char>();
                return Property::VariantT{c};
            }
            case PropertyType::ENUM: return Property::VariantT{static_cast<uint8_t>(n.as<int>())};
            default: throw std::runtime_error("Unsupported property type for deserialization");
        }
    }
} // BreadEngine
