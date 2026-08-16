#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentTonemapParameters : InspectorStruct
    {
        TonemapMode mode = TonemapMode::Linear;       ///< Tone mapping algorithm
        float exposure = 1.0f;      ///< Scene brightness multiplier
        float white = 1.0f;         ///< Reference white point (not used for AGX)

        EnvironmentTonemapParameters() = default;

        ~EnvironmentTonemapParameters() override = default;

    private:
        INSPECTOR_BEGIN(EnvironmentTonemapParameters)
            INSPECT_FIELD(mode)
            INSPECT_FIELD(exposure)
            INSPECT_FIELD(white)
        INSPECTOR_END()
    };
} // BreadEngine
