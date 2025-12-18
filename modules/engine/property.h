#pragma once
#include <functional>
#include <string>
#include <variant>
#include "raylib.h"

namespace BreadEngine {
    struct Component;

    enum class PropertyType : uint8_t { COMPONENT, INT, FLOAT, LONG, BOOL, STRING, VECTOR2, VECTOR3, VECTOR4, COLOR, ENUM };

    struct Property
    {
        using VariantT = std::variant<Component, int, float, long, bool, std::string, Vector2, Vector3, Vector4, Color, uint8_t>;
        std::string name;
        PropertyType type;
        std::function<VariantT(const Component *)> get;
        std::function<void(Component *, const VariantT &)> set;
        std::function<std::string(const VariantT &)> toStr;
    };
} // BreadEngine

#define PROP_STRING(COMP, PROP) \
    auto val = PROP.get(&COMP); \
    auto display = PROP.name + ": " + PROP.toStr(val); \
    TraceLog(LOG_DEBUG, "%s", display.c_str());
