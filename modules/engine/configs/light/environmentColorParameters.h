#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentColorParameters : InspectorStruct
    {
        float brightness = 1.0f; ///< Overall brightness multiplier
        float contrast = 1.0f; ///< Contrast between dark and bright areas
        float saturation = 1.0f; ///< Color intensity

        EnvironmentColorParameters() = default;

        ~EnvironmentColorParameters() override = default;

    private:
        INSPECTOR_BEGIN(EnvironmentColorParameters)
            INSPECT_FIELD(brightness)
            INSPECT_FIELD(contrast)
            INSPECT_FIELD(saturation)
        INSPECTOR_END()
    };
} // BreadEngine
