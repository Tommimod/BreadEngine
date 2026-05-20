#pragma once
#include "inspectorObject.h"

namespace BreadEngine {
    struct SkyboxProceduralParameters : InspectorStruct
    {
        Color skyTopColor;
        Color skyHorizonColor;
        ///0 - 1 range
        float skyHorizonCurve;
        float skyEnergy;

        Color groundBottomColor;
        Color groundHorizonColor;
        ///0 - 1 range
        float groundHorizonCurve;
        float groundEnergy;

        Vector3 sunDirection;
        Color sunColor;
        float sunSize;
        float sunCurve;
        float sunEnergy;

        bool isCreated = false;

        SkyboxProceduralParameters() = default;

        ~SkyboxProceduralParameters() override = default;

        static SkyboxProceduralParameters defaultParameters();

    private:
        INSPECTOR_BEGIN(SkyboxProceduralParameters)
            INSPECT_FIELD(skyTopColor)
            INSPECT_FIELD(skyHorizonColor)
            INSPECT_FIELD(skyHorizonCurve)
            INSPECT_FIELD(skyEnergy)

            INSPECT_FIELD(groundBottomColor)
            INSPECT_FIELD(groundHorizonColor)
            INSPECT_FIELD(groundHorizonCurve)
            INSPECT_FIELD(groundEnergy)

            INSPECT_FIELD(sunDirection)
            INSPECT_FIELD(sunColor)
            INSPECT_FIELD(sunSize)
            INSPECT_FIELD(sunCurve)
            INSPECT_FIELD(sunEnergy)
        INSPECTOR_END()
    };
} // BreadEngine
