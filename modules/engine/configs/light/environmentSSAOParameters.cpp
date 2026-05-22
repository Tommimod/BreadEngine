#include "environmentSSAOParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentSSAOParameters)

    EnvironmentSSAOParameters &EnvironmentSSAOParameters::fromNative(const R3D_EnvSSAO &nativeData)
    {
        sampleCount = nativeData.sampleCount;
        intensity = nativeData.intensity;
        power = nativeData.power;
        radius = nativeData.radius;
        bias = nativeData.bias;
        enabled = nativeData.enabled;
        return *this;
    }

    R3D_EnvSSAO EnvironmentSSAOParameters::toNative() const
    {
        return R3D_EnvSSAO{
            .sampleCount = sampleCount,
            .intensity = intensity,
            .power = power,
            .radius = radius,
            .bias = bias,
            .enabled = enabled
        };
    }
} // BreadEngine
