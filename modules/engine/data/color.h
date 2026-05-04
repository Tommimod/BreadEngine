#pragma once
#include <cstdint>

#include "inspectorObject.h"
#include "raylib.h"

namespace BreadEngine {
    struct Color : InspectorStruct
    {
        Color() = default;

        Color(const int r, const int g, const int b, const int a = 255) : r(r), g(g), b(b), a(a)
        {
        }

        int r = 255, g = 255, b = 255, a = 255;

        [[nodiscard]] Vector3 asVector3() const
        {
            return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
        }

        [[nodiscard]] Vector4 asVector4() const
        {
            return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a)};
        }

        bool operator==(const Color &color) const
        {
            return r == color.r && g == color.g && b == color.b && a == color.a;
        }

        bool operator!=(const Color &color) const
        {
            return !(*this == color);
        }

        INSPECTOR_BEGIN(Color)
            INSPECT_FIELD(r);
            INSPECT_FIELD(g);
            INSPECT_FIELD(b);
            INSPECT_FIELD(a);
        INSPECTOR_END()
    };
} // BreadEngine
