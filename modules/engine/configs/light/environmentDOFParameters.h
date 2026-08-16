#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentDOFParameters : InspectorStruct
    {
        DepthOfFieldMode mode = DepthOfFieldMode::Disabled;           ///< Enable/disable state
        float focusPoint = 10.0f;   ///< Focus distance in meters from camera
        float focusScale = 1.0f;    ///< Depth of field depth: lower = shallower
        float nearScale = 1.0f;     ///< Near blur intensity: 0.0 = disabled, 1.0 = symmetric to far
        float maxBlurSize = 20.0f;  ///< Maximum blur radius, similar to aperture

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
