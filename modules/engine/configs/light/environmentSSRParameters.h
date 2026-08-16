#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentSSRParameters : InspectorStruct
    {
        float stepSize = 0.125f; ///< Ray step size
        float thickness = 0.2f; ///< Depth tolerance for valid hits
        float maxDistance = 4.0f; ///< Maximum ray distance
        float edgeFade = 0.25f; ///< Screen edge fade start [0,1]
        int maxRaySteps = 32; ///< Maximum ray marching steps
        int binarySteps = 4; ///< Binary search refinement steps
        bool enabled = false; ///< Enable/disable SSR

        EnvironmentSSRParameters() = default;

        ~EnvironmentSSRParameters() override = default;

    private:
        INSPECTOR_BEGIN(EnvironmentSSRParameters)
            INSPECT_FIELD(enabled)
            INSPECT_FIELD(stepSize)
            INSPECT_FIELD(thickness)
            INSPECT_FIELD(maxDistance)
            INSPECT_FIELD(edgeFade)
            INSPECT_FIELD(maxRaySteps)
            INSPECT_FIELD(binarySteps)
        INSPECTOR_END()
    };
} // BreadEngine
