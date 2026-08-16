#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentSSGIParameters : InspectorStruct
    {
        float stepSize = 0.125f; ///< Ray step size
        float thickness = 1.0f; ///< Depth tolerance for valid hits
        float maxDistance = 4.0f; ///< Maximum ray distance
        float intensity = 3.0f; ///< GI intensity multiplier
        float fadeStart = 8.0f; ///< Distance at which the GI fade begins
        float fadeEnd = 16.0f; ///< Distance at which GI is fully faded
        int sampleCount = 2; ///< Number of rays per pixel
        int maxRaySteps = 32; ///< Maximum ray marching steps
        int denoiseSteps = 5; ///< Number of denoiser iterations
        bool enabled = false; ///< Enable/disable SSGI

        EnvironmentSSGIParameters() = default;

        ~EnvironmentSSGIParameters() override = default;

    private:
        INSPECTOR_BEGIN(EnvironmentSSGIParameters)
            INSPECT_FIELD(enabled)
            INSPECT_FIELD(stepSize)
            INSPECT_FIELD(maxDistance)
            INSPECT_FIELD(intensity)
            INSPECT_FIELD(fadeStart)
            INSPECT_FIELD(fadeEnd)
            INSPECT_FIELD(sampleCount)
            INSPECT_FIELD(maxRaySteps)
            INSPECT_FIELD(denoiseSteps)
        INSPECTOR_END()
    };
} // BreadEngine
