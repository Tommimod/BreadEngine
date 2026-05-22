#include "environmentSSGIParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentSSGIParameters)

    EnvironmentSSGIParameters &EnvironmentSSGIParameters::fromNative(const R3D_EnvSSGI &nativeData)
    {
        sampleCount = nativeData.sampleCount;
        maxRaySteps = nativeData.maxRaySteps;
        stepSize = nativeData.stepSize;
        thickness = nativeData.thickness;
        maxDistance = nativeData.maxDistance;
        intensity = nativeData.intensity;
        fadeStart = nativeData.fadeStart;
        fadeEnd = nativeData.fadeEnd;
        denoiseSteps = nativeData.denoiseSteps;
        enabled = nativeData.enabled;
        return *this;
    }

    R3D_EnvSSGI EnvironmentSSGIParameters::toNative() const
    {
        return R3D_EnvSSGI{
            .sampleCount = sampleCount,
            .maxRaySteps = maxRaySteps,
            .stepSize = stepSize,
            .thickness = thickness,
            .maxDistance = maxDistance,
            .intensity = intensity,
            .fadeStart = fadeStart,
            .fadeEnd = fadeEnd,
            .denoiseSteps = denoiseSteps,
            .enabled = enabled
        };
    }
} // BreadEngine
