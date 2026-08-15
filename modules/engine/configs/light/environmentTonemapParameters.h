#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentTonemapParameters : InspectorStruct
    {
        TonemapMode mode = TonemapMode::Linear;       ///< Tone mapping algorithm
        float exposure = 0;         ///< Scene brightness multiplier (default: 1.0)
        float white = 0;            ///< Reference white point (not used for AGX) (default: 1.0)

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
