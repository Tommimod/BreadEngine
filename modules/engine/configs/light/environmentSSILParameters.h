#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentSSILParameters : InspectorStruct
    {
        float radius = 2.0f; ///< Maximum distance to gather light from
        float thickness = 1.0f; ///< Thickness threshold for occluders
        float intensity = 1.0f; ///< IL intensity multiplier
        float aoPower = 1.0f; ///< AO exponent/power
        int sampleCount = 2; ///< Number of samples to compute indirect lighting
        int sliceCount = 4; ///< Number of depth slices for accumulation
        int denoiseSteps = 4; ///< Number of denoiser iterations
        bool enabled = false; ///< Enable/disable SSIL effect

        EnvironmentSSILParameters() = default;

        ~EnvironmentSSILParameters() override = default;

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
