#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentSSAOParameters : InspectorStruct
    {
        float intensity = 0; ///< Base occlusion strength multiplier (default: 1.0)
        float power = 0; ///< Exponential falloff for sharper darkening (default: 1.5)
        float radius = 0; ///< Sampling radius in world space (default: 0.25)
        float bias = 0; ///< Depth bias to prevent self-shadowing, good value is ~2% of the radius (default: 0.007)
        int sampleCount = 0; ///< Number of samples to compute SSAO (default: 16)
        bool enabled = false; ///< Enable/disable SSAO effect (default: false)

        EnvironmentSSAOParameters() = default;

        ~EnvironmentSSAOParameters() override = default;

        EnvironmentSSAOParameters &fromNative(const R3D_EnvSSAO &nativeData);

        [[nodiscard]] R3D_EnvSSAO toNative() const;

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
