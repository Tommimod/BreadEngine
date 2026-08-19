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
        float height = 0.0f; ///< World height the fog is at full density up to
        float heightFalloff = 0.0f; ///< How fast the fog thins above that height; 0 fills the world evenly
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
            INSPECT_FIELD(height)
            INSPECT_FIELD(heightFalloff)
            INSPECT_FIELD(skyAffect)
        INSPECTOR_END()
    };
} // BreadEngine
