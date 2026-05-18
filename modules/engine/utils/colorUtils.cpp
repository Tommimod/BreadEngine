#include "colorUtils.h"

namespace BreadEngine {
    bool ColorUtils::IsCompare(const Color &color1, const Color &color2)
    {
        return color1.r == color2.r && color1.g == color2.g && color1.b == color2.b && color1.a == color2.a;
    }

    Vector3 ColorUtils::ToVector3(const Color &color)
    {
        return Vector3{static_cast<float>(color.r), static_cast<float>(color.g), static_cast<float>(color.b)};
    }

    Vector4 ColorUtils::ToVector4(const Color &color)
    {
        return Vector4{static_cast<float>(color.r), static_cast<float>(color.g), static_cast<float>(color.b), static_cast<float>(color.a)};
    }
} // BreadEngine
