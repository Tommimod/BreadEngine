#pragma once
#include <any>
#include <chrono>
#include <functional>
#include <random>
#include <string>
#include <typeindex>
#include "raylib.h"
#include <type_traits>
#include <utility>
#include <yaml-cpp/yaml.h>
#include <magic_enum/magic_enum.hpp>

namespace BreadEngine {
    struct InspectorStruct;

    enum class PropertyType : uint8_t { INSPECTOR_STRUCT, INT, FLOAT, LONG, BOOL, STRING, VECTOR2, VECTOR3, VECTOR4, COLOR, ENUM, VECTOR_L, NODE_LINK, ASSET_LINK };

    template<typename T>
    struct DeducePropertyType
    {
        static constexpr PropertyType value = []
        {
            if constexpr (std::is_base_of_v<InspectorStruct, T>)
            {
                return PropertyType::INSPECTOR_STRUCT;
            }
            else if constexpr (std::is_enum_v<T>)
            {
                return PropertyType::ENUM;
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

    struct Component;

    struct Asset;

    template<typename T>
    struct DeducePropertyType<T *>
    {
        static constexpr PropertyType value = []() constexpr
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                return PropertyType::NODE_LINK;
            }
            else if constexpr (std::is_base_of_v<Asset, T>)
            {
                return PropertyType::ASSET_LINK;
            }
            else
            {
                static_assert(false, "Unsupported pointer type for inspector property");
                return PropertyType{};
            }
        }();
    };

    template<typename T>
    struct DeducePropertyType<std::shared_ptr<T> >
    {
        static constexpr PropertyType value = []() constexpr
        {
            if constexpr (std::is_base_of_v<InspectorStruct, T>)
            {
                return PropertyType::INSPECTOR_STRUCT;
            }
            else
            {
                static_assert(false, "Unsupported shared_ptr type for inspector property. Only InspectorStruct-derived types are supported for shared_ptr.");
                return PropertyType{};
            }
        }();
    };

    template<typename T>
    struct is_shared_ptr : std::false_type
    {
    };

    template<typename T>
    struct is_shared_ptr<std::shared_ptr<T> > : std::true_type
    {
    };

    template<typename T>
    constexpr PropertyType deducePropertyType()
    {
        return DeducePropertyType<T>::value;
    }

    struct Property
    {
        enum Options
        {
            NONE = 0,
            READONLY = 1 << 0,
            HIDDEN = 1 << 1
        };

        using VariantT = std::any;
        std::string guid;
        std::string name;
        std::string prettyName;
        PropertyType type;
        PropertyType elementType = PropertyType{};
        std::type_index expectedComponentType{typeid(void)};
        std::type_index expectedAssetType{typeid(void)};
        std::function<VariantT(const InspectorStruct *)> get;
        std::function<void(InspectorStruct *, const VariantT &)> set;
        std::function<std::string(const VariantT &)> toStr;
        std::function<std::vector<std::string>()> getEnumNames{};
        std::function<VariantT(int)> enumIndexToValue{};
        Options options = NONE;
        bool isConditional = false;
        std::function<bool(const InspectorStruct *)> conditionFunc;
        bool oldConditionState = false;

        [[nodiscard]] bool isReadOnly() const { return options & READONLY; }
        [[nodiscard]] bool isHidden() const { return options & HIDDEN; }

        [[nodiscard]] bool isConditionSatisfied(const InspectorStruct *obj)
        {
            if (!isConditional)
            {
                return true;
            }

            auto newState = conditionFunc(obj);
            const auto isChanged = newState != oldConditionState;
            oldConditionState = std::move(newState);
            return isChanged;
        }

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
            using ValueType = VecT::value_type;
            if constexpr (std::is_base_of_v<InspectorStruct, ValueType>)
            {
                return std::any{const_cast<InspectorStruct *>(static_cast<const InspectorStruct *>(&vec[index]))};
            }
            else if constexpr (is_shared_ptr<ValueType>::value)
            {
                using ElementType = ValueType::element_type;
                if constexpr (std::is_base_of_v<InspectorStruct, ElementType>)
                {
                    return std::any{const_cast<InspectorStruct *>(static_cast<const InspectorStruct *>(vec[index].get()))};
                }
                else if constexpr (std::is_enum_v<ElementType>)
                {
                    auto enumIndex = magic_enum::enum_index(*vec[index]);
                    return std::any{static_cast<int>(enumIndex.value_or(0))};
                }
                else
                {
                    return std::any{vec[index]};
                }
            }
            else if constexpr (std::is_enum_v<ValueType>)
            {
                auto enumIndex = magic_enum::enum_index(vec[index]);
                return std::any{static_cast<int>(enumIndex.value_or(0))};
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
            using ValueType = VecT::value_type;
            if constexpr (is_shared_ptr<ValueType>::value)
            {
                using ElementType = ValueType::element_type;
                if constexpr (std::is_enum_v<ElementType>)
                {
                    const int enumIndex = std::any_cast<int>(value);
                    if (auto enumValue = magic_enum::enum_cast<ElementType>(static_cast<size_t>(enumIndex)); enumValue.has_value()) *vec[index] = enumValue.value();
                }
                else
                {
                    vec[index] = std::any_cast<ValueType>(value);
                }
            }
            else if constexpr (std::is_enum_v<ValueType>)
            {
                const int enumIndex = std::any_cast<int>(value);
                if (auto enumValue = magic_enum::enum_cast<ValueType>(static_cast<size_t>(enumIndex)); enumValue.has_value()) vec[index] = enumValue.value();
            }
            else
            {
                vec[index] = std::any_cast<ValueType>(value);
            }
        }

        void add() override
        {
            using ValueType = VecT::value_type;
            if constexpr (is_shared_ptr<ValueType>::value)
            {
                using ElementType = ValueType::element_type;
                vec.push_back(std::make_shared<ElementType>());
            }
            else
            {
                vec.push_back({});
            }
        }

        void remove(size_t index) override
        {
            if (index >= vec.size())
            {
                throw std::out_of_range("Vector index out of range");
            }
            vec.erase(vec.begin() + index);
        }

        void forEachInspectorStruct(const std::function<void(InspectorStruct *)> callback) override
        {
            using ValueType = VecT::value_type;
            if constexpr (std::is_base_of_v<InspectorStruct, ValueType>)
            {
                for (auto &elem: vec)
                {
                    callback(static_cast<InspectorStruct *>(&elem));
                }
            }
            else if constexpr (is_shared_ptr<ValueType>::value)
            {
                using ElementType = ValueType::element_type;
                if constexpr (std::is_base_of_v<InspectorStruct, ElementType>)
                {
                    for (auto &elem: vec)
                    {
                        callback(static_cast<InspectorStruct *>(elem.get()));
                    }
                }
            }
        }

        [[nodiscard]] PropertyType elementType() const override
        {
            return deducePropertyType<typename VecT::value_type>();
        }
    };

    struct DeferredNodeLink
    {
        unsigned int sourceOwnerId = 0;
        std::string sourceComponentType;
        std::string propertyPath;
        unsigned int targetNodeId = 0;
        std::string targetComponentType;
    };

    struct DeferredAssetLink
    {
        unsigned int sourceOwnerId = 0;
        std::string sourceComponentType;
        std::string propertyPath;
        std::string targetAssetGuid;
        std::string targetAssetType;
    };

    struct InspectorStruct
    {
        bool isChangedFromEditor = false;

        virtual ~InspectorStruct() = default;

        [[nodiscard]] virtual std::vector<Property> &getInspectedProperties() const
        {
            static std::vector<Property> empty{};
            return empty;
        }

        [[nodiscard]] std::string getTypeName();

        static std::string getNewGUID();

        static std::string getPrettyFieldName(const std::string &fieldName);

        virtual YAML::Node serialize();

        void deserialize(const YAML::Node &node);

        void deserialize(const YAML::Node &node, const std::string &parentPath);

        static void beginDeserializationPhase();

        static void endDeserializationPhase();

        static bool isInDeserializationPhase();

        static void resolveAllDeferredNodeLinks();

        static void resolveAllDeferredAssetLinks();

        static bool setPropertyByPath(InspectorStruct *obj, const std::string &path, const Property::VariantT &value, PropertyType expectedType);

        static void setCurrentDeserializingComponent(Component *comp);

        static Component *getCurrentDeserializingComponent();

        static void setCurrentDeserializingOwnerId(unsigned int ownerId);

        static unsigned int getCurrentDeserializingOwnerId();

    protected:
        static YAML::Node propertyToYaml(const Property &prop, const Property::VariantT &val);

        static Property::VariantT yamlToVariant(PropertyType type, const YAML::Node &n, const std::string &propertyName);

        static Property::VariantT yamlToVariant(PropertyType type, const YAML::Node &node);

    private:
        static std::vector<DeferredNodeLink> &getDeferredNodeLinks();

        static std::vector<DeferredAssetLink> &getDeferredAssetLinks();

        static bool &getDeserializationFlag();

        static Component *&getCurrentComponent();

        static unsigned int &getCurrentOwnerId();
    };

    template<typename Class, typename Field>
    using member_type_t = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Class>().*std::declval<Field *>())> >;

    template<typename MemberPtr>
    using member_pointer_result_t = std::remove_cv_t<std::remove_reference_t<
        decltype(std::declval<std::remove_pointer_t<std::decay_t<MemberPtr> > >()
                 .*std::declval<std::decay_t<MemberPtr> >())> >;

    template<typename LocalClass, typename T>
    void addInspectedProperty(std::vector<Property> &props, const char *fieldName, T LocalClass::*memberPtr, Property::Options options, std::function<bool(const InspectorStruct *)> conditionFunc);

    template<typename LocalClass, typename T>
    void addInspectedProperty(std::vector<Property> &props, const char *fieldName, T LocalClass::*memberPtr)
    {
        addInspectedProperty(props, fieldName, memberPtr, Property::Options::NONE, nullptr);
    }

    template<typename LocalClass, typename T>
    void addInspectedProperty(std::vector<Property> &props, const char *fieldName, T LocalClass::*memberPtr, Property::Options options)
    {
        addInspectedProperty(props, fieldName, memberPtr, options, nullptr);
    }

    template<typename LocalClass, typename T>
    void addInspectedProperty(std::vector<Property> &props, const char *fieldName, T LocalClass::*memberPtr, std::function<bool(const InspectorStruct *)> conditionFunc)
    {
        addInspectedProperty(props, fieldName, memberPtr, Property::Options::NONE, conditionFunc);
    }

    template<typename LocalClass, typename T>
    void addInspectedProperty(std::vector<Property> &props, const char *fieldName, T LocalClass::*memberPtr, const Property::Options options, const std::function<bool(const InspectorStruct *)> conditionFunc)
    {
        using RawFieldType = std::remove_cvref_t<decltype( static_cast<LocalClass *>(nullptr)->*memberPtr )>;
        using FieldType = std::conditional_t<std::is_reference_v<RawFieldType>, std::remove_reference_t<RawFieldType>, RawFieldType>;

        if constexpr (constexpr PropertyType ptype = deducePropertyType<FieldType>(); ptype == PropertyType::INSPECTOR_STRUCT)
        {
            if constexpr (is_shared_ptr<FieldType>::value)
            {
                props.emplace_back(Property{
                    .guid = InspectorStruct::getNewGUID(),
                    .name = std::string(fieldName),
                    .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
                    .type = ptype,
                    .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                    {
                        auto nonConstComp = const_cast<InspectorStruct *>(comp);
                        auto nonConstObj = static_cast<LocalClass *>(nonConstComp);
                        auto &sharedPtr = nonConstObj->*memberPtr;
                        return static_cast<InspectorStruct *>(sharedPtr.get());
                    },
                    .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                    {
                        auto *obj = static_cast<LocalClass *>(comp);
                        obj->*memberPtr = std::any_cast<FieldType>(value);
                        comp->isChangedFromEditor = true;
                    },
                    .toStr = [](const Property::VariantT &val) -> std::string
                    {
                        if (val.type() == typeid(std::string)) return std::any_cast<std::string>(val);
                        if (val.type() == typeid(uint8_t)) return std::to_string(std::any_cast<uint8_t>(val));
                        if (val.type() == typeid(int)) return std::to_string(std::any_cast<int>(val));
                        if (val.type() == typeid(float)) return std::to_string(std::any_cast<float>(val));
                        if (val.type() == typeid(long)) return std::to_string(std::any_cast<long>(val));
                        if (val.type() == typeid(bool)) return std::any_cast<bool>(val) ? "true" : "false";
                        if (val.type() == typeid(Vector2)) return std::to_string(std::any_cast<Vector2>(val).x) + "," + std::to_string(std::any_cast<Vector2>(val).y);
                        if (val.type() == typeid(Vector3)) return std::to_string(std::any_cast<Vector3>(val).x) + "," + std::to_string(std::any_cast<Vector3>(val).y) + "," + std::to_string(std::any_cast<Vector3>(val).z);
                        if (val.type() == typeid(Vector4)) return std::to_string(std::any_cast<Vector4>(val).x) + "," + std::to_string(std::any_cast<Vector4>(val).y) + "," + std::to_string(std::any_cast<Vector4>(val).z) + "," + std::to_string(std::any_cast<Vector4>(val).w);
                        if (val.type() == typeid(Color)) return std::to_string(std::any_cast<Color>(val).r) + "," + std::to_string(std::any_cast<Color>(val).g) + "," + std::to_string(std::any_cast<Color>(val).b) + "," + std::to_string(std::any_cast<Color>(val).a);
                        return "[unsupported]";
                    },
                    .options = options,
                    .isConditional = conditionFunc != nullptr,
                    .conditionFunc = conditionFunc
                });
            }
            else
            {
                props.emplace_back(Property{
                    .guid = InspectorStruct::getNewGUID(),
                    .name = std::string(fieldName),
                    .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
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
                        obj->*memberPtr = std::any_cast<FieldType>(value);
                        comp->isChangedFromEditor = true;
                    },
                    .toStr = [](const Property::VariantT &val) -> std::string
                    {
                        if (val.type() == typeid(std::string)) return std::any_cast<std::string>(val);
                        if (val.type() == typeid(uint8_t)) return std::to_string(std::any_cast<uint8_t>(val));
                        if (val.type() == typeid(int)) return std::to_string(std::any_cast<int>(val));
                        if (val.type() == typeid(float)) return std::to_string(std::any_cast<float>(val));
                        if (val.type() == typeid(long)) return std::to_string(std::any_cast<long>(val));
                        if (val.type() == typeid(bool)) return std::any_cast<bool>(val) ? "true" : "false";
                        if (val.type() == typeid(Vector2)) return std::to_string(std::any_cast<Vector2>(val).x) + "," + std::to_string(std::any_cast<Vector2>(val).y);
                        if (val.type() == typeid(Vector3)) return std::to_string(std::any_cast<Vector3>(val).x) + "," + std::to_string(std::any_cast<Vector3>(val).y) + "," + std::to_string(std::any_cast<Vector3>(val).z);
                        if (val.type() == typeid(Vector4)) return std::to_string(std::any_cast<Vector4>(val).x) + "," + std::to_string(std::any_cast<Vector4>(val).y) + "," + std::to_string(std::any_cast<Vector4>(val).z) + "," + std::to_string(std::any_cast<Vector4>(val).w);
                        if (val.type() == typeid(Color)) return std::to_string(std::any_cast<Color>(val).r) + "," + std::to_string(std::any_cast<Color>(val).g) + "," + std::to_string(std::any_cast<Color>(val).b) + "," + std::to_string(std::any_cast<Color>(val).a);
                        return "[unsupported]";
                    },
                    .options = options,
                    .isConditional = conditionFunc != nullptr,
                    .conditionFunc = conditionFunc
                });
            }
        }
        else if constexpr (ptype == PropertyType::VECTOR_L)
        {
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
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
                    comp->isChangedFromEditor = true;
                },
                .toStr = [](const Property::VariantT &) -> std::string
                {
                    return "[vector]";
                },
                .options = options,
                .isConditional = conditionFunc != nullptr,
                .conditionFunc = conditionFunc
            });
        }
        else if constexpr (ptype == PropertyType::ENUM)
        {
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
                .type = ptype,
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    const auto *obj = static_cast<const LocalClass *>(comp);
                    auto value = obj->*memberPtr;
                    auto index = magic_enum::enum_index(value);
                    return Property::VariantT{static_cast<int>(index.value_or(0))};
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    const int index = std::any_cast<int>(value);
                    if (auto enumValue = magic_enum::enum_cast<FieldType>(static_cast<size_t>(index)); enumValue.has_value()) obj->*memberPtr = enumValue.value();
                    comp->isChangedFromEditor = true;
                },
                .toStr = {},
                .getEnumNames = []() -> std::vector<std::string>
                {
                    std::vector<std::string> names;
                    for (auto name: magic_enum::enum_names<FieldType>()) names.emplace_back(std::string(name));
                    return names;
                },
                .enumIndexToValue = [](const int index) -> Property::VariantT
                {
                    if (auto enumValue = magic_enum::enum_cast<FieldType>(static_cast<size_t>(index)); enumValue.has_value()) return Property::VariantT{static_cast<std::underlying_type_t<FieldType>>(enumValue.value())};
                    return Property::VariantT{};
                },
                .options = options,
                .isConditional = conditionFunc != nullptr,
                .conditionFunc = conditionFunc
            });
        }
        else if constexpr (ptype == PropertyType::NODE_LINK)
        {
            using ComponentType = std::remove_pointer_t<FieldType>;
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
                .type = ptype,
                .expectedComponentType = std::type_index(typeid(ComponentType)),
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    const auto *obj = static_cast<const LocalClass *>(comp);
                    Component *value = obj->*memberPtr;
                    return Property::VariantT{value};
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    if (value.has_value())
                    {
                        if (value.type() == typeid(Component *))
                        {
                            auto compPtr = std::any_cast<Component *>(value);
                            obj->*memberPtr = dynamic_cast<FieldType>(compPtr);
                        }
                        else if (value.type() == typeid(std::shared_ptr<Component>))
                        {
                            const auto compSharedPtr = std::any_cast<std::shared_ptr<Component> >(value);
                            obj->*memberPtr = dynamic_cast<FieldType>(compSharedPtr.get());
                        }
                        else
                        {
                            obj->*memberPtr = nullptr;
                        }
                    }
                    else
                    {
                        obj->*memberPtr = nullptr;
                    }

                    comp->isChangedFromEditor = true;
                },
                .toStr = {},
                .options = options,
                .isConditional = conditionFunc != nullptr,
                .conditionFunc = conditionFunc
            });
        }
        else if constexpr (ptype == PropertyType::ASSET_LINK)
        {
            using AssetType = std::remove_pointer_t<FieldType>;
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
                .type = ptype,
                .expectedAssetType = std::type_index(typeid(AssetType)),
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    const auto *obj = static_cast<const LocalClass *>(comp);
                    Asset *value = obj->*memberPtr;
                    return Property::VariantT{value};
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    if (value.has_value())
                    {
                        if (value.type() == typeid(Asset *))
                        {
                            auto assetPtr = std::any_cast<Asset *>(value);
                            obj->*memberPtr = dynamic_cast<FieldType>(assetPtr);
                        }
                        else if (value.type() == typeid(std::shared_ptr<Asset>))
                        {
                            const auto assetSharedPtr = std::any_cast<std::shared_ptr<Asset> >(value);
                            obj->*memberPtr = dynamic_cast<FieldType>(assetSharedPtr.get());
                        }
                        else
                        {
                            obj->*memberPtr = nullptr;
                        }
                    }
                    else
                    {
                        obj->*memberPtr = nullptr;
                    }

                    comp->isChangedFromEditor = true;
                },
                .toStr = {},
                .options = options,
                .isConditional = conditionFunc != nullptr,
                .conditionFunc = conditionFunc
            });
        }
        else
        {
            props.emplace_back(Property{
                .guid = InspectorStruct::getNewGUID(),
                .name = std::string(fieldName),
                .prettyName = InspectorStruct::getPrettyFieldName(fieldName),
                .type = ptype,
                .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT
                {
                    const auto *obj = static_cast<const LocalClass *>(comp);
                    auto value = obj->*memberPtr;
                    return Property::VariantT{value};
                },
                .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value)
                {
                    auto *obj = static_cast<LocalClass *>(comp);
                    obj->*memberPtr = std::any_cast<FieldType>(value);
                    comp->isChangedFromEditor = true;
                },
                .toStr = {},
                .options = options,
                .isConditional = conditionFunc != nullptr,
                .conditionFunc = conditionFunc
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
#define INSPECT_FIELD_OPT(FieldName, FieldOptions) \
            addInspectedProperty<LocalClass>(props, #FieldName, &LocalClass::FieldName, FieldOptions);
#define INSPECT_FIELD_COND(FieldName, ConditionBoolFunc) \
            addInspectedProperty<LocalClass>(props, #FieldName, &LocalClass::FieldName, [](const InspectorStruct *obj) -> bool { \
                auto *localObj = dynamic_cast<const LocalClass *>(obj); \
                return ConditionBoolFunc(localObj); \
            });
#define INSPECTOR_END() \
            return props; \
        }

#define DEFINE_STATIC_PROPS(ClassName) \
    std::vector<Property> ClassName::inspectorProps_;
} // BreadEngine

#define PROP_STRING(COMP, PROP) \
    auto val = PROP.get(&COMP); \
    auto display = PROP.name + ": " + PROP.toStr(val); \
    Logger::LogInfo(display);
