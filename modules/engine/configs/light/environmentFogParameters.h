#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentFogParameters : InspectorStruct
    {
        FogMode mode = FogMode::Disabled; ///< Fog distribution mode
        Color color = WHITE; ///< Fog tint color
        float start = 1.0f; ///< Linear mode: distance where fog begins
        float end = 50.0f; ///< Linear mode: distance of full fog density
        float density = 0.05f; ///< Exponential modes: fog thickness factor
        float skyAffect = 0.5f; ///< Fog influence on skybox [0-1]

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
