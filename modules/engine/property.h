#pragma once
#include <functional>
#include <string>
#include <variant>
#include "raylib.h"

namespace BreadEngine {
    struct Component;

    enum class PropertyType { COMPONENT, INT, FLOAT, LONG, BOOL, STRING, VECTOR2, VECTOR3, VECTOR4, COLOR };

    struct Property
    {
        std::string name;
        PropertyType type;
        std::function<std::variant<Component, int, float, long, bool, std::string, Vector2, Vector3, Vector4, Color>(const Component *)> get;
        std::function<void(Component *, const std::variant<Component, int, float, long, bool, std::string, Vector2, Vector3, Vector4, Color> &)> set;
        std::function<std::string(const std::variant<Component, int, float, long, bool, std::string, Vector2, Vector3, Vector4, Color>&)> toStr;
    };
} // BreadEngine
