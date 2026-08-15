#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentDOFParameters : InspectorStruct
    {
        DepthOfFieldMode mode = DepthOfFieldMode::Disabled;           ///< Enable/disable state
        float focusPoint = 0;       ///< Focus distance in meters from camera (default: 10.0)
        float focusScale = 0;       ///< Depth of field depth: lower = shallower (default: 1.0)
        float nearScale = 0;        ///< Near blur intensity: 0.0 = disabled, 1.0 = symmetric to far (default: 1.0)
        float maxBlurSize = 0;      ///< Maximum blur radius, similar to aperture (default: 20.0)

        EnvironmentDOFParameters() = default;

        ~EnvironmentDOFParameters() override = default;

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
