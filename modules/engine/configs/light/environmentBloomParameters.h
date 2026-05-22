#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentBloomParameters : InspectorStruct
    {
        R3D_Bloom mode = R3D_BLOOM_DISABLED; ///< Bloom blending mode (default: R3D_BLOOM_DISABLED)
        float levels = 0; ///< Mipmap spread factor [0-1]: higher = wider glow (default: 0.5)
        float intensity = 0; ///< Bloom strength multiplier (default: 0.05)
        float threshold = 0; ///< Minimum brightness to trigger bloom (default: 0.0)
        float softThreshold = 0; ///< Softness of brightness cutoff transition (default: 0.5)
        float filterRadius = 0; ///< Blur filter radius during upscaling (default: 1.0)

        EnvironmentBloomParameters() = default;

        ~EnvironmentBloomParameters() override = default;

        EnvironmentBloomParameters &fromNative(const R3D_EnvBloom &nativeData);

        [[nodiscard]] R3D_EnvBloom toNative() const;

    private:
        INSPECTOR_BEGIN(EnvironmentBloomParameters)
            INSPECT_FIELD(mode)
            INSPECT_FIELD(levels)
            INSPECT_FIELD(intensity)
            INSPECT_FIELD(threshold)
            INSPECT_FIELD(softThreshold)
            INSPECT_FIELD(filterRadius)
        INSPECTOR_END()
    };
} // BreadEngine
