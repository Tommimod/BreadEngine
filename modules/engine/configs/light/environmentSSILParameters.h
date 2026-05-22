#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentSSILParameters : InspectorStruct
    {
        float radius = 0; ///< Maximum distance to gather light from (default: 2.0)
        float thickness = 0; ///< Thickness threshold for occluders (default: 1.0)
        float intensity = 0; ///< IL intensity multiplier (default: 1.0)
        float aoPower = 0; ///< AO exponent/power (default: 1.0)
        int sampleCount = 0; ///< Number of samples to compute indirect lighting (default: 2)
        int sliceCount = 0; ///< Number of depth slices for accumulation (default: 4)
        int denoiseSteps = 0; ///< Number of denoiser iterations (default: 4)
        bool enabled = false; ///< Enable/disable SSIL effect (default: false)

        EnvironmentSSILParameters() = default;

        ~EnvironmentSSILParameters() override = default;

        EnvironmentSSILParameters &fromNative(const R3D_EnvSSIL &nativeData);

        [[nodiscard]] R3D_EnvSSIL toNative() const;

    private:
        INSPECTOR_BEGIN(EnvironmentSSILParameters)
            INSPECT_FIELD(enabled)
            INSPECT_FIELD(radius)
            INSPECT_FIELD(thickness)
            INSPECT_FIELD(intensity)
            INSPECT_FIELD(aoPower)
            INSPECT_FIELD(sampleCount)
            INSPECT_FIELD(sliceCount)
            INSPECT_FIELD(denoiseSteps)
        INSPECTOR_END()
    };
} // BreadEngine
