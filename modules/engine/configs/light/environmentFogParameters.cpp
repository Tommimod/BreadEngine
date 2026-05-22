#include "environmentFogParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentFogParameters)

    EnvironmentFogParameters &EnvironmentFogParameters::fromNative(const R3D_EnvFog &nativeData)
    {
        mode = nativeData.mode;
        color = nativeData.color;
        start = nativeData.start;
        end = nativeData.end;
        density = nativeData.density;
        skyAffect = nativeData.skyAffect;
        return *this;
    }

    R3D_EnvFog EnvironmentFogParameters::toNative() const
    {
        return R3D_EnvFog{
            .mode = mode,
            .color = color,
            .start = start,
            .end = end,
            .density = density,
            .skyAffect = skyAffect
        };
    }
} // BreadEngine
