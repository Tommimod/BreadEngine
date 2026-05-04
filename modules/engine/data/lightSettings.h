#pragma once
#include "inspectorObject.h"
#include "r3d_lighting.h"
#include "data/color.h"

namespace BreadEngine {
    struct LightSettings : InspectorStruct
    {
        LightSettings()
        {
            lightType = R3D_LIGHT_DIR;
            color = Color{255, 255, 255, 255};
            withShadows = true;
        }

        bool operator==(const LightSettings &other) const
        {
            return lightType == other.lightType &&
                   color == other.color &&
                   withShadows == other.withShadows;
        }

        bool operator!=(const LightSettings &other) const
        {
            return !(*this == other);
        }

        R3D_LightType lightType;
        Color color;
        bool withShadows;

        INSPECTOR_BEGIN(LightSettings)
            INSPECT_FIELD(lightType);
            INSPECT_FIELD(withShadows);
            INSPECT_FIELD(color);
        INSPECTOR_END()
    };
} // BreadEngine
