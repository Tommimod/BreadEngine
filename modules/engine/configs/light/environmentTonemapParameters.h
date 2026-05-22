#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentTonemapParameters : InspectorStruct
    {
        R3D_Tonemap mode = R3D_TONEMAP_LINEAR;       ///< Tone mapping algorithm (default: R3D_TONEMAP_LINEAR)
        float exposure = 0;         ///< Scene brightness multiplier (default: 1.0)
        float white = 0;            ///< Reference white point (not used for AGX) (default: 1.0)

        EnvironmentTonemapParameters() = default;

        ~EnvironmentTonemapParameters() override = default;

        EnvironmentTonemapParameters &fromNative(const R3D_EnvTonemap &nativeData);

        [[nodiscard]] R3D_EnvTonemap toNative() const;

    private:
        INSPECTOR_BEGIN(EnvironmentTonemapParameters)
            INSPECT_FIELD(mode)
            INSPECT_FIELD(exposure)
            INSPECT_FIELD(white)
        INSPECTOR_END()
    };
} // BreadEngine
