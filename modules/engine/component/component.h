#pragma once
#include <string>
#include <cxxabi.h>
#include <type_traits>
#include <functional>
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

    protected:
        friend class NodeInspector;
        Node *_parent = nullptr;
        bool _isActive = true;

    private:
        [[nodiscard]] virtual std::vector<Property> getInspectedPropertiesImpl() const
        {
            return {};
        }

        [[nodiscard]] std::string getTypeName();
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

#define INSPECTOR_BEGIN(ClassName) \
    private: \
        static std::vector<Property> inspectorProps_; \
        [[nodiscard]] std::vector<Property> getInspectedPropertiesImpl() const override { \
            if (inspectorProps_.empty()) { \
                inspectorProps_ = buildInspectedProps(); \
            } \
            return inspectorProps_; \
        } \
        static std::vector<Property> buildInspectedProps() { \
            std::vector<Property> props; \
            using LocalClass = ClassName;

#define INSPECT_FIELD(FieldName, MemberPtr) \
            { \
                constexpr auto memberPtr = MemberPtr; \
                using RawFieldType = std::remove_cvref_t<decltype( (static_cast<LocalClass*>(nullptr)->*memberPtr) )>; \
                using FieldType = std::conditional_t<std::is_reference_v<RawFieldType>, std::remove_reference_t<RawFieldType>, RawFieldType>; \
                static_assert(std::is_same_v<FieldType, Component> || std::is_same_v<FieldType, int> || \
                              std::is_same_v<FieldType, float> || std::is_same_v<FieldType, long> || \
                              std::is_same_v<FieldType, bool> || std::is_same_v<FieldType, std::string> || \
                              std::is_same_v<FieldType, Vector2> || std::is_same_v<FieldType, Vector3> || \
                              std::is_same_v<FieldType, Vector4> || std::is_same_v<FieldType, Color>, \
                              "FieldType not supported in VariantT"); \
                constexpr PropertyType ptype = deducePropertyType<FieldType>(); \
                using VariantT = std::variant<Component, int, float, long, bool, std::string, Vector2, Vector3, Vector4, Color>; \
                props.emplace_back(Property{ \
                    std::string(#FieldName), \
                    ptype, \
                    [=](const Component* comp) -> VariantT { \
                        if (!comp) return VariantT{}; \
                        const auto* typed = dynamic_cast<const LocalClass*>(comp); \
                        if (!typed) return VariantT{}; \
                        if constexpr (std::is_same_v<FieldType, Component>) { \
                            return static_cast<const FieldType&>(typed->*memberPtr); \
                        } else { \
                            return static_cast<FieldType>(typed->*memberPtr); \
                        } \
                    }, \
                    [=](Component* comp, const VariantT& val) { \
                        if (!comp) return; \
                        auto* typed = dynamic_cast<LocalClass*>(comp); \
                        if (!typed) return; \
                        std::visit([typed, memberPtr](auto&& arg) -> void { \
                            using ArgType = std::decay_t<decltype(arg)>; \
                            if constexpr (std::is_same_v<ArgType, FieldType>) { \
                                typed->*memberPtr = std::forward<decltype(arg)>(arg); \
                            } else { \
                                throw std::runtime_error("Type mismatch in setter for " + std::string(#FieldName)); \
                            } \
                        }, val); \
                    }, \
                    [=](const VariantT& val) -> std::string { \
                        return std::visit([](auto&& arg) -> std::string { \
                            using ArgType = std::decay_t<decltype(arg)>; \
                            if constexpr (std::is_same_v<ArgType, int>) return std::to_string(arg); \
                            else if constexpr (std::is_same_v<ArgType, float>) return std::to_string(arg); \
                            else if constexpr (std::is_same_v<ArgType, long>) return std::to_string(arg); \
                            else if constexpr (std::is_same_v<ArgType, bool>) return arg ? "true" : "false"; \
                            else if constexpr (std::is_same_v<ArgType, std::string>) return arg; \
                            else if constexpr (std::is_same_v<ArgType, Vector2>) return TextFormat("%f, %f", arg.x, arg.y); \
                            else if constexpr (std::is_same_v<ArgType, Vector3>) return TextFormat("%f, %f, %f", arg.x, arg.y, arg.z); \
                            else if constexpr (std::is_same_v<ArgType, Vector4>) return TextFormat("%f, %f, %f, %f", arg.x, arg.y, arg.z, arg.w); \
                            else if constexpr (std::is_same_v<ArgType, Color>) return TextFormat("R:%d G:%d B:%d A:%d", arg.r, arg.g, arg.b, arg.a); \
                            else if constexpr (std::is_same_v<ArgType, Component>) return arg.toString(); \
                            else return "N/A"; \
                        }, val); \
                    } \
                }); \
            }
#define INSPECTOR_END() \
            return props; \
        } \
    public: \
        [[nodiscard]] std::vector<Property> getInspectedProperties() const { \
            return getInspectedPropertiesImpl(); \
        } \
    private:

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

#define DEFINE_STATIC_PROPS(ClassName) \
    std::vector<Property> ClassName::inspectorProps_;
} // namespace BreadEngine
