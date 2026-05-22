#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentColorParameters : InspectorStruct
    {
        float brightness = 0; ///< Overall brightness multiplier (default: 1.0)
        float contrast = 0; ///< Contrast between dark and bright areas (default: 1.0)
        float saturation = 0; ///< Color intensity (default: 1.0)

        EnvironmentColorParameters() = default;

        ~EnvironmentColorParameters() override = default;

        EnvironmentColorParameters &fromNative(const R3D_EnvColor &nativeData);

        [[nodiscard]] R3D_EnvColor toNative() const;

    private:
        INSPECTOR_BEGIN(EnvironmentColorParameters)
            INSPECT_FIELD(brightness)
            INSPECT_FIELD(contrast)
            INSPECT_FIELD(saturation)
        INSPECTOR_END()
    };
} // BreadEngine
