#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentDOFParameters : InspectorStruct
    {
        R3D_DoF mode = R3D_DOF_DISABLED;           ///< Enable/disable state (default: R3D_DOF_DISABLED)
        float focusPoint = 0;       ///< Focus distance in meters from camera (default: 10.0)
        float focusScale = 0;       ///< Depth of field depth: lower = shallower (default: 1.0)
        float nearScale = 0;        ///< Near blur intensity: 0.0 = disabled, 1.0 = symmetric to far (default: 1.0)
        float maxBlurSize = 0;      ///< Maximum blur radius, similar to aperture (default: 20.0)

        EnvironmentDOFParameters() = default;

        ~EnvironmentDOFParameters() override = default;

        EnvironmentDOFParameters &fromNative(const R3D_EnvDoF &nativeData);

        [[nodiscard]] R3D_EnvDoF toNative() const;

    private:
        INSPECTOR_BEGIN(EnvironmentDOFParameters)
            INSPECT_FIELD(mode)
            INSPECT_FIELD(focusPoint)
            INSPECT_FIELD(focusScale)
            INSPECT_FIELD(nearScale)
            INSPECT_FIELD(maxBlurSize)
        INSPECTOR_END()
    };
} // BreadEngine
