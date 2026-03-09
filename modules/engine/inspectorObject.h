#pragma once
#include <any>
#include <chrono>
#include <functional>
#include <iomanip>
#include <random>
#include <string>
#include "raylib.h"
#include <type_traits>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace BreadEngine {
    struct InspectorStruct;

    enum class PropertyType : uint8_t { INSPECTOR_STRUCT, INT, FLOAT, LONG, BOOL, STRING, VECTOR2, VECTOR3, VECTOR4, COLOR, ENUM, VECTOR_L };

    template<typename T>
    struct DeducePropertyType
    {
        static constexpr PropertyType value = []
        {
            if constexpr (std::is_base_of_v<InspectorStruct, T>)
            {
                return PropertyType::INSPECTOR_STRUCT;
            }
            else
            {
                static_assert(false, "Unsupported type for inspector property");
                return PropertyType{};
            }
        }();
    };

    template<>
    struct DeducePropertyType<int>
    {
        static constexpr auto value = PropertyType::INT;
    };

    template<>
    struct DeducePropertyType<float>
    {
        static constexpr auto value = PropertyType::FLOAT;
    };

    template<>
    struct DeducePropertyType<long>
    {
        static constexpr auto value = PropertyType::LONG;
    };

    template<>
    struct DeducePropertyType<bool>
    {
        static constexpr auto value = PropertyType::BOOL;
    };

    template<>
    struct DeducePropertyType<std::string>
    {
        static constexpr auto value = PropertyType::STRING;
    };

    template<>
    struct DeducePropertyType<Vector2>
    {
        static constexpr auto value = PropertyType::VECTOR2;
    };

    template<>
    struct DeducePropertyType<Vector3>
    {
        static constexpr auto value = PropertyType::VECTOR3;
    };

    template<>
    struct DeducePropertyType<Vector4>
    {
        static constexpr auto value = PropertyType::VECTOR4;
    };

    template<>
    struct DeducePropertyType<Color>
    {
        static constexpr auto value = PropertyType::COLOR;
    };

    template<>
    struct DeducePropertyType<uint8_t>
    {
        static constexpr auto value = PropertyType::ENUM;
    };

    template<typename T>
    struct DeducePropertyType<std::vector<T> >
    {
        static constexpr PropertyType value = []() constexpr
        {
            return PropertyType::VECTOR_L;
        }();
    };

    template<typename T>
    constexpr PropertyType deducePropertyType()
    {
        return DeducePropertyType<T>::value;
    }

    struct Property
    {
        using VariantT = std::any;
        std::string guid;
        std::string name;
        PropertyType type;
        PropertyType elementType = PropertyType{};
        std::function<VariantT(const InspectorStruct *)> get;
        std::function<void(InspectorStruct *, const VariantT &)> set;
        std::function<std::string(const VariantT &)> toStr;

        bool operator==(const Property &other) const
        {
            return guid == other.guid && name == other.name && type == other.type && elementType == other.elementType;
        }

        bool operator!=(const Property &other) const
        {
            return !(*this == other);
        }
    };

    struct VectorAccessor
    {
        virtual ~VectorAccessor() = default;

        [[nodiscard]] virtual size_t size() const = 0;

        [[nodiscard]] virtual std::any get(size_t index) const = 0;

        virtual void set(size_t index, const std::any &value) = 0;

        virtual void add() = 0;

        virtual void remove(size_t index) = 0;

        virtual void forEachInspectorStruct(std::function<void(InspectorStruct *)> callback) = 0;

        [[nodiscard]] virtual PropertyType elementType() const = 0;
    };

    template<typename VecT>
    struct TypedVectorAccessor : VectorAccessor
    {
        VecT &vec;

        explicit TypedVectorAccessor(VecT &v) : vec(v)
        {
        }

        [[nodiscard]] size_t size() const override
        {
            return vec.size();
        }

        [[nodiscard]] std::any get(size_t index) const override
        {
            if (index >= vec.size())
            {
                throw std::out_of_range("Vector index out of range");
            }
            if constexpr (std::is_base_of_v<InspectorStruct, typename VecT::value_type>)
            {
                return std::any{const_cast<InspectorStruct *>(static_cast<const InspectorStruct *>(&vec[index]))};
            }
            else
            {
                return std::any{vec[index]};
            }
        }

        void set(size_t index, const std::any &value) override
        {
            if (index >= vec.size())
            {
                throw std::out_of_range("Vector index out of range");
            }
            vec[index] = std::any_cast<typename VecT::value_type>(value);
        }

        void add() override
        {
            vec.push_back({});
        }

        void remove(size_t index) override
        {
            if (index >= vec.size())
            {
                throw std::out_of_range("Vector index out of range");
            }
            vec.erase(vec.begin() + index);
        }

        void forEachInspectorStruct(std::function<void(InspectorStruct *)> callback) override
        {
            if constexpr (std::is_base_of_v<InspectorStruct, typename VecT::value_type>)
            {
                for (auto &elem: vec)
                {
                    callback(static_cast<InspectorStruct *>(&elem));
                }
            }
        }

        [[nodiscard]] PropertyType elementType() const override
        {
            return deducePropertyType<typename VecT::value_type>();
        }
    };

    struct InspectorStruct
    {
        virtual ~InspectorStruct() = default;

        [[nodiscard]] virtual std::vector<Property> &getInspectedProperties() const
        {
            static std::vector<Property> empty{};
            return empty;
        }

        [[nodiscard]] std::string getTypeName();

        static std::string getNewGUID()
        {
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

        virtual YAML::Node serialize()
        {
            YAML::Node node(YAML::NodeType::Map);
            for (const auto &prop: getInspectedProperties())
            {
                auto val = prop.get(this);
                node[prop.name] = propertyToYaml(prop, val);
            }
            return node;
        }

        void deserialize(const YAML::Node &node)
        {
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

    protected:
        static YAML::Node propertyToYaml(const Property &prop, const Property::VariantT &val)
        {
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

        static Property::VariantT yamlToVariant(const PropertyType type, const YAML::Node &n)
        {
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
    };

    template<typename Class, typename Field>
    using member_type_t = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Class>().*std::declval<Field *>())> >;

    template<typename MemberPtr>
    using member_pointer_result_t = std::remove_cv_t<std::remove_reference_t<
        decltype((std::declval<std::remove_pointer_t<std::decay_t<MemberPtr> > >()
                  .*std::declval<std::decay_t<MemberPtr> >()))> >;

    template<typename LocalClass, typename T>
    void addInspectedProperty(std::vector<Property> &props, const char *fieldName, T LocalClass::*memberPtr)
    {
        using RawFieldType = std::remove_cvref_t<decltype( (static_cast<LocalClass *>(nullptr)->*memberPtr) )>;
        using FieldType = std::conditional_t<std::is_reference_v<RawFieldType>, std::remove_reference_t<RawFieldType>, RawFieldType>;
        constexpr PropertyType ptype = deducePropertyType<FieldType>();

        if constexpr (ptype == PropertyType::INSPECTOR_STRUCT)
        {
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .type = ptype,
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    auto nonConstComp = const_cast<InspectorStruct *>(comp);
                    auto nonConstObj = static_cast<LocalClass *>(nonConstComp);
                    auto *structPtr = static_cast<InspectorStruct *>(&(nonConstObj->*memberPtr));
                    return structPtr;
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    obj->*memberPtr = std::any_cast<FieldType>(value); //TODO RISKI
                },
                .toStr = [](const Property::VariantT &val) -> std::string
                {
                    if (val.type() == typeid(std::string)) return std::any_cast<std::string>(val);
                    if (val.type() == typeid(int)) return std::to_string(std::any_cast<int>(val));
                    if (val.type() == typeid(float)) return std::to_string(std::any_cast<float>(val));
                    if (val.type() == typeid(long)) return std::to_string(std::any_cast<long>(val));
                    if (val.type() == typeid(bool)) return std::any_cast<bool>(val) ? "true" : "false";
                    if (val.type() == typeid(Vector2)) return std::to_string(std::any_cast<Vector2>(val).x) + "," + std::to_string(std::any_cast<Vector2>(val).y);
                    if (val.type() == typeid(Vector3)) return std::to_string(std::any_cast<Vector3>(val).x) + "," + std::to_string(std::any_cast<Vector3>(val).y) + "," + std::to_string(std::any_cast<Vector3>(val).z);
                    if (val.type() == typeid(Vector4)) return std::to_string(std::any_cast<Vector4>(val).x) + "," + std::to_string(std::any_cast<Vector4>(val).y) + "," + std::to_string(std::any_cast<Vector4>(val).z) + "," + std::to_string(std::any_cast<Vector4>(val).w);
                    if (val.type() == typeid(Color)) return std::to_string(std::any_cast<Color>(val).r) + "," + std::to_string(std::any_cast<Color>(val).g) + "," + std::to_string(std::any_cast<Color>(val).b) + "," + std::to_string(std::any_cast<Color>(val).a);
                    return "[unsupported]";
                }
            });
        }
        else if constexpr (ptype == PropertyType::VECTOR_L)
        {
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .type = ptype,
                .elementType = deducePropertyType<typename FieldType::value_type>(),
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    auto *nonConstObj = const_cast<LocalClass *>(static_cast<const LocalClass *>(comp));
                    auto &vecRef = nonConstObj->*memberPtr;
                    using VecT = std::remove_reference_t<decltype(vecRef)>;

                    std::shared_ptr<VectorAccessor> acc = std::make_shared<TypedVectorAccessor<VecT> >(vecRef);
                    return acc;
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    obj->*memberPtr = std::any_cast<FieldType>(value);
                },
                .toStr = [](const Property::VariantT &) -> std::string
                {
                    return "[vector]";
                }
            });
        }
        else
        {
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .type = ptype,
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    const auto *obj = static_cast<const LocalClass *>(comp);
                    return Property::VariantT{obj->*memberPtr};
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    obj->*memberPtr = std::any_cast<FieldType>(value);
                },
                .toStr = {}
            });
        }
    }

#define INSPECTOR_BEGIN(ClassName) \
    private: \
        static std::vector<Property> inspectorProps_; \
        [[nodiscard]] std::vector<Property> &getInspectedProperties() const override { \
            if (inspectorProps_.empty()) { \
                inspectorProps_ = buildInspectedProps(); \
            } \
            return inspectorProps_; \
        } \
        static std::vector<Property> buildInspectedProps() { \
            std::vector<Property> props; \
            using LocalClass = ClassName;

#define INSPECT_FIELD(FieldName) \
            addInspectedProperty<LocalClass>(props, #FieldName, &LocalClass::FieldName);
#define INSPECTOR_END() \
            return props; \
        }

#define DEFINE_STATIC_PROPS(ClassName) \
    std::vector<Property> ClassName::inspectorProps_;
} // BreadEngine

#define PROP_STRING(COMP, PROP) \
    auto val = PROP.get(&COMP); \
    auto display = PROP.name + ": " + PROP.toStr(val); \
    TraceLog(LOG_DEBUG, "%s", display.c_str());
