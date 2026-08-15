#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentFogParameters : InspectorStruct
    {
        FogMode mode = FogMode::Disabled; ///< Fog distribution mode
        Color color = WHITE; ///< Fog tint color (default: white)
        float start = 0; ///< Linear mode: distance where fog begins (default: 1.0)
        float end = 0; ///< Linear mode: distance of full fog density (default: 50.0)
        float density = 0; ///< Exponential modes: fog thickness factor (default: 0.05)
        float skyAffect = 0; ///< Fog influence on skybox [0-1] (default: 0.5)

        EnvironmentFogParameters() = default;

        ~EnvironmentFogParameters() override = default;

    private:
        INSPECTOR_BEGIN(EnvironmentFogParameters)
            INSPECT_FIELD(mode)
            INSPECT_FIELD(color)
            INSPECT_FIELD(start)
            INSPECT_FIELD(end)
            INSPECT_FIELD(density)
            INSPECT_FIELD(skyAffect)
        INSPECTOR_END()
    };
} // BreadEngine
