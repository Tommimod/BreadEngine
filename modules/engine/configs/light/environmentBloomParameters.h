#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentBloomParameters : InspectorStruct
    {
        BloomMode mode = BloomMode::Disabled; ///< Bloom blending mode
        float levels = 0.5f; ///< Mipmap spread factor [0-1]: higher = wider glow
        float intensity = 0.05f; ///< Bloom strength multiplier
        float threshold = 0; ///< Minimum brightness to trigger bloom
        float softThreshold = 0.5f; ///< Softness of brightness cutoff transition
        float filterRadius = 1.0f; ///< Blur filter radius during upscaling

        EnvironmentBloomParameters() = default;

        ~EnvironmentBloomParameters() override = default;

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
