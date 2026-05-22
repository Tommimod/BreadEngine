#include "environmentColorParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentColorParameters)

    EnvironmentColorParameters &EnvironmentColorParameters::fromNative(const R3D_EnvColor &nativeData)
    {
        brightness = nativeData.brightness;
        contrast = nativeData.contrast;
        saturation = nativeData.saturation;
        return *this;
    }

    R3D_EnvColor EnvironmentColorParameters::toNative() const
    {
        return R3D_EnvColor{
            .brightness = brightness,
            .contrast = contrast,
            .saturation = saturation
        };
    }
} // BreadEngine
