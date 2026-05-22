#include "environmentBloomParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentBloomParameters)

    EnvironmentBloomParameters &EnvironmentBloomParameters::fromNative(const R3D_EnvBloom &nativeData)
    {
        mode = nativeData.mode;
        levels = nativeData.levels;
        intensity = nativeData.intensity;
        threshold = nativeData.threshold;
        softThreshold = nativeData.softThreshold;
        filterRadius = nativeData.filterRadius;
        return *this;
    }

    R3D_EnvBloom EnvironmentBloomParameters::toNative() const
    {
        return R3D_EnvBloom{
            .mode = mode,
            .levels = levels,
            .intensity = intensity,
            .threshold = threshold,
            .softThreshold = softThreshold,
            .filterRadius = filterRadius
        };
    }
} // BreadEngine
