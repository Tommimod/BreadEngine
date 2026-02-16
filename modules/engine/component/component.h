#pragma once
#include <string>
#include <type_traits>
#include <utility>
#include "property.h"

namespace BreadEngine {
    class Node;

    struct Component
    {
        Component() = default;

        void setOwner(Node *parent);

        virtual ~Component();

        virtual void init();

        virtual void start();

        virtual void update(float deltaTime);

        virtual void fixedUpdate(float fixedDeltaTime);

        virtual void onFrameStart(float deltaTime);

        virtual void onFrameEnd(float deltaTime);

        virtual void destroy();

        [[nodiscard]] virtual std::string toString() const;

        [[nodiscard]] Node *getParent() const;

        [[nodiscard]] bool getIsActive() const;

        virtual void setIsActive(bool nextActive);

        void setParent(Node *nextParent);

        [[nodiscard]] virtual std::vector<Property> getInspectedProperties() const
        {
            return {};
        }

        [[nodiscard]] std::string getTypeName();

    protected:
        Node *_parent = nullptr;
        bool _isActive = true;
    };

    template<typename T>
    constexpr PropertyType deducePropertyType()
    {
        return {};
    }

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
        static_assert(std::is_same_v<FieldType, Component> || std::is_same_v<FieldType, int> ||
                      std::is_same_v<FieldType, float> || std::is_same_v<FieldType, long> ||
                      std::is_same_v<FieldType, bool> || std::is_same_v<FieldType, std::string> ||
                      std::is_same_v<FieldType, Vector2> || std::is_same_v<FieldType, Vector3> ||
                      std::is_same_v<FieldType, Vector4> || std::is_same_v<FieldType, Color> ||
                      std::is_same_v<FieldType, uint8_t>,
                      "FieldType not supported in VariantT");
        constexpr PropertyType ptype = deducePropertyType<FieldType>();
        props.emplace_back(Property{
            std::string(fieldName),
            ptype,
            [memberPtr](const Component *comp) -> Property::VariantT
            {
                const auto *obj = static_cast<const LocalClass *>(comp);
                return Property::VariantT{obj->*memberPtr};
            },
            [memberPtr](Component *comp, const Property::VariantT &value)
            {
                auto *obj = static_cast<LocalClass *>(comp);
                if constexpr (std::is_same_v<FieldType, int>)
                {
                    obj->*memberPtr = std::get<int>(value);
                }
                else if constexpr (std::is_same_v<FieldType, float>)
                {
                    obj->*memberPtr = std::get<float>(value);
                }
                else if constexpr (std::is_same_v<FieldType, long>)
                {
                    obj->*memberPtr = std::get<long>(value);
                }
                else if constexpr (std::is_same_v<FieldType, bool>)
                {
                    obj->*memberPtr = std::get<bool>(value);
                }
                else if constexpr (std::is_same_v<FieldType, std::string>)
                {
                    obj->*memberPtr = std::get<std::string>(value);
                }
                else if constexpr (std::is_same_v<FieldType, Vector2>)
                {
                    obj->*memberPtr = std::get<Vector2>(value);
                }
                else if constexpr (std::is_same_v<FieldType, Vector3>)
                {
                    obj->*memberPtr = std::get<Vector3>(value);
                }
                else if constexpr (std::is_same_v<FieldType, Vector4>)
                {
                    obj->*memberPtr = std::get<Vector4>(value);
                }
                else if constexpr (std::is_same_v<FieldType, Color>)
                {
                    obj->*memberPtr = std::get<Color>(value);
                }
                else if constexpr (std::is_same_v<FieldType, uint8_t>)
                {
                    obj->*memberPtr = std::get<uint8_t>(value);
                }
                else if constexpr (std::is_same_v<FieldType, Component>)
                {
                    obj->*memberPtr = std::get<Component>(value);
                }
            }
        });
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

    template<>
    constexpr PropertyType deducePropertyType<Component>() { return PropertyType::COMPONENT; }

    template<>
    constexpr PropertyType deducePropertyType<int>() { return PropertyType::INT; }

    template<>
    constexpr PropertyType deducePropertyType<float>() { return PropertyType::FLOAT; }

    template<>
    constexpr PropertyType deducePropertyType<long>() { return PropertyType::LONG; }

    template<>
    constexpr PropertyType deducePropertyType<bool>() { return PropertyType::BOOL; }

    template<>
    constexpr PropertyType deducePropertyType<std::string>() { return PropertyType::STRING; }

    template<>
    constexpr PropertyType deducePropertyType<Vector2>() { return PropertyType::VECTOR2; }

    template<>
    constexpr PropertyType deducePropertyType<Vector3>() { return PropertyType::VECTOR3; }

    template<>
    constexpr PropertyType deducePropertyType<Vector4>() { return PropertyType::VECTOR4; }

    template<>
    constexpr PropertyType deducePropertyType<Color>() { return PropertyType::COLOR; }

    template<>
    constexpr PropertyType deducePropertyType<uint8_t>() { return PropertyType::ENUM; }

#define DEFINE_STATIC_PROPS(ClassName) \
    std::vector<Property> ClassName::inspectorProps_;
} // namespace BreadEngine
