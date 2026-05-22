#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentFogParameters : InspectorStruct
    {
        R3D_Fog mode = R3D_FOG_DISABLED; ///< Fog distribution mode (default: R3D_FOG_DISABLED)
        Color color = WHITE; ///< Fog tint color (default: white)
        float start = 0; ///< Linear mode: distance where fog begins (default: 1.0)
        float end = 0; ///< Linear mode: distance of full fog density (default: 50.0)
        float density = 0; ///< Exponential modes: fog thickness factor (default: 0.05)
        float skyAffect = 0; ///< Fog influence on skybox [0-1] (default: 0.5)

        EnvironmentFogParameters() = default;

        ~EnvironmentFogParameters() override = default;

        EnvironmentFogParameters &fromNative(const R3D_EnvFog &nativeData);

        [[nodiscard]] R3D_EnvFog toNative() const;

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
