#pragma once
#include "raylib.h"

namespace BreadEngine {
    class ColorUtils
    {
    public:
        static bool IsCompare(const Color &color1, const Color &color2);
        static Vector3 ToVector3(const Color &color);
        static Vector4 ToVector4(const Color &color);
    };
} // BreadEngine
