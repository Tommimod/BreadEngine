#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentSSAOParameters : InspectorStruct
    {
        float intensity = 0.5f; ///< Base occlusion strength multiplier
        float power = 1.5f; ///< Exponential falloff for sharper darkening
        float radius = 0.5f; ///< Sampling radius in world space
        float bias = 0.02f; ///< Depth bias to prevent self-shadowing, good value is ~2% of the radius
        int sampleCount = 16; ///< Number of samples to compute SSAO
        bool enabled = false; ///< Enable/disable SSAO effect

        EnvironmentSSAOParameters() = default;

        ~EnvironmentSSAOParameters() override = default;

    private:
        INSPECTOR_BEGIN(EnvironmentSSAOParameters)
            INSPECT_FIELD(enabled)
            INSPECT_FIELD(sampleCount)
            INSPECT_FIELD(intensity)
            INSPECT_FIELD(power)
            INSPECT_FIELD(radius)
            INSPECT_FIELD(bias)
        INSPECTOR_END()
    };
} // BreadEngine
