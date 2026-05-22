#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentSSGIParameters : InspectorStruct
    {
        float stepSize = 0; ///< Ray step size (default: 0.125)
        float thickness = 0; ///< Depth tolerance for valid hits (default: 1.0)
        float maxDistance = 0; ///< Maximum ray distance (default: 4.0)
        float intensity = 0; ///< GI intensity multiplier (default: 3.0)
        float fadeStart = 0; ///< Distance at which the GI fade begins (default: 8.0)
        float fadeEnd = 0; ///< Distance at which GI is fully faded (default: 16.0)
        int sampleCount = 0; ///< Number of rays per pixel (default: 2)
        int maxRaySteps = 0; ///< Maximum ray marching steps (default: 32)
        int denoiseSteps = 0; ///< Number of denoiser iterations (default: 5)
        bool enabled = false; ///< Enable/disable SSGI (default: false)

        EnvironmentSSGIParameters() = default;

        ~EnvironmentSSGIParameters() override = default;

        EnvironmentSSGIParameters &fromNative(const R3D_EnvSSGI &nativeData);

        [[nodiscard]] R3D_EnvSSGI toNative() const;

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
