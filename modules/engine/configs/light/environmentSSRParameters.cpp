#include "environmentSSRParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentSSRParameters)

    EnvironmentSSRParameters &EnvironmentSSRParameters::fromNative(const R3D_EnvSSR &nativeData)
    {
        maxRaySteps = nativeData.maxRaySteps;
        binarySteps = nativeData.binarySteps;
        stepSize = nativeData.stepSize;
        thickness = nativeData.thickness;
        maxDistance = nativeData.maxDistance;
        edgeFade = nativeData.edgeFade;
        enabled = nativeData.enabled;
        return *this;
    }

    R3D_EnvSSR EnvironmentSSRParameters::toNative() const
    {
        return R3D_EnvSSR{
            .maxRaySteps = maxRaySteps,
            .binarySteps = binarySteps,
            .stepSize = stepSize,
            .thickness = thickness,
            .maxDistance = maxDistance,
            .edgeFade = edgeFade,
            .enabled = enabled
        };
    }
} // BreadEngine
