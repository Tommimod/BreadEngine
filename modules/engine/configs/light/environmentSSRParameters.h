#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentSSRParameters : InspectorStruct
    {
        float stepSize = 0; ///< Ray step size (default: 0.125)
        float thickness = 0; ///< Depth tolerance for valid hits (default: 0.2)
        float maxDistance = 0; ///< Maximum ray distance (default: 4.0)
        float edgeFade = 0; ///< Screen edge fade start [0,1] (default: 0.25)
        int maxRaySteps = 0; ///< Maximum ray marching steps (default: 32)
        int binarySteps = 0; ///< Binary search refinement steps (default: 4)
        bool enabled = false; ///< Enable/disable SSR (default: false)

        EnvironmentSSRParameters() = default;

        ~EnvironmentSSRParameters() override = default;

        EnvironmentSSRParameters &fromNative(const R3D_EnvSSR &nativeData);

        [[nodiscard]] R3D_EnvSSR toNative() const;

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
