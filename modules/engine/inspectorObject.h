#pragma once
#include <any>
#include <functional>
#include <string>
#include "raylib.h"
#include <type_traits>
#include <utility>

namespace BreadEngine {
    struct InspectorStruct;

    enum class PropertyType : uint8_t { INSPECTOR_STRUCT, INT, FLOAT, LONG, BOOL, STRING, VECTOR2, VECTOR3, VECTOR4, COLOR, ENUM, VECTOR_L };

    template<typename T>
    struct DeducePropertyType {
        static constexpr PropertyType value = [] {
            if constexpr (std::is_base_of_v<InspectorStruct, T>) {
                return PropertyType::INSPECTOR_STRUCT;
            } else {
                static_assert(false, "Unsupported type for inspector property");
                return PropertyType{}; // never reached
            }
        }();
    };

    template<>
    struct DeducePropertyType<int> {
        static constexpr auto value = PropertyType::INT;
    };

    template<>
    struct DeducePropertyType<float> {
        static constexpr auto value = PropertyType::FLOAT;
    };

    template<>
    struct DeducePropertyType<long> {
        static constexpr auto value = PropertyType::LONG;
    };

    template<>
    struct DeducePropertyType<bool> {
        static constexpr auto value = PropertyType::BOOL;
    };

    template<>
    struct DeducePropertyType<std::string> {
        static constexpr auto value = PropertyType::STRING;
    };

    template<>
    struct DeducePropertyType<Vector2> {
        static constexpr auto value = PropertyType::VECTOR2;
    };

    template<>
    struct DeducePropertyType<Vector3> {
        static constexpr auto value = PropertyType::VECTOR3;
    };

    template<>
    struct DeducePropertyType<Vector4> {
        static constexpr auto value = PropertyType::VECTOR4;
    };

    template<>
    struct DeducePropertyType<Color> {
        static constexpr auto value = PropertyType::COLOR;
    };

    template<>
    struct DeducePropertyType<uint8_t> {
        static constexpr auto value = PropertyType::ENUM;
    };

    template<typename T>
    struct DeducePropertyType<std::vector<T>> {
        static constexpr PropertyType value = []() constexpr {
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
        std::string name;
        PropertyType type;
        std::function<VariantT(const InspectorStruct *)> get;
        std::function<void(InspectorStruct *, const VariantT &)> set;
        std::function<std::string(const VariantT &)> toStr;
    };

    struct InspectorStruct
    {
        virtual ~InspectorStruct() = default;

        [[nodiscard]] virtual std::vector<Property> getInspectedProperties() const
        {
            return {};
        }

        [[nodiscard]] std::string getTypeName();
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

    if constexpr (ptype == PropertyType::INSPECTOR_STRUCT) {
        props.emplace_back(Property{
            .name = std::string(fieldName),
            .type = ptype,
            .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT {
                auto nonConstComp = const_cast<InspectorStruct*>(comp);
                auto nonConstObj = static_cast<LocalClass*>(nonConstComp);
                InspectorStruct* structPtr = static_cast<InspectorStruct*>(&(nonConstObj->*memberPtr));
                return structPtr;
            },
            .set = [](InspectorStruct *, const Property::VariantT &) {
                // Не используем set для вложенных структур — редактируем рекурсивно
                // Если нужно присвоить целиком, реализуйте, но рискуете slicing
            },
            .toStr = {}  // Если нужно, добавьте реализацию
        });
    } else if constexpr (ptype == PropertyType::VECTOR_L) {
        props.emplace_back(Property{
            .name = std::string(fieldName),
            .type = ptype,
            .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT {
                auto nonConstComp = const_cast<InspectorStruct*>(comp);
                auto nonConstObj = static_cast<LocalClass*>(nonConstComp);
                void* vecPtr = static_cast<void*>(&(nonConstObj->*memberPtr));
                auto forEach = [vecPtr](auto callback) {
                    auto& vec = *static_cast<FieldType*>(vecPtr);
                    for (auto& elem : vec) {
                        callback(&elem);
                    }
                };
                return std::make_pair(vecPtr, forEach);
            },
            .set = [](InspectorStruct *, const Property::VariantT &) {
            },
            .toStr = {}
        });
    } else {
        props.emplace_back(Property{
            .name = std::string(fieldName),
            .type = ptype,
            .get = [memberPtr](const InspectorStruct *comp) -> Property::VariantT {
                const auto *obj = static_cast<const LocalClass *>(comp);
                return Property::VariantT{obj->*memberPtr};
            },
            .set = [memberPtr](InspectorStruct *comp, const Property::VariantT &value) {
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
        [[nodiscard]] std::vector<Property> getInspectedProperties() const override { \
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
