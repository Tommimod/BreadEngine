#include "environmentSSILParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentSSILParameters)

    EnvironmentSSILParameters &EnvironmentSSILParameters::fromNative(const R3D_EnvSSIL &nativeData)
    {
        radius = nativeData.radius;
        thickness = nativeData.thickness;
        intensity = nativeData.intensity;
        aoPower = nativeData.aoPower;
        sampleCount = nativeData.sampleCount;
        sliceCount = nativeData.sliceCount;
        denoiseSteps = nativeData.denoiseSteps;
        enabled = nativeData.enabled;
        return *this;
    }

    R3D_EnvSSIL EnvironmentSSILParameters::toNative() const
    {
        return R3D_EnvSSIL{
            .sampleCount = sampleCount,
            .sliceCount = sliceCount,
            .radius = radius,
            .thickness = thickness,
            .intensity = intensity,
            .aoPower = aoPower,
            .denoiseSteps = denoiseSteps,
            .enabled = enabled
        };
    }
} // BreadEngine
