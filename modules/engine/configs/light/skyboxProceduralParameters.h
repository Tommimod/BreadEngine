#pragma once
#include "inspectorObject.h"

namespace BreadEngine {
    /**
     * Conditions the analytic sky dome is built from. Everything the model itself takes is
     * physical; the two tint-and-energy fields sit on top of its result, because a purely
     * physical daytime sky cannot express a night, a moon or anything that is not Earth.
     *
     * The sun is not authored here. Direction, colour and energy are taken from the scene's
     * directional light every frame, so the sky and the light that casts the shadows cannot
     * disagree about where the sun is.
     */
    struct SkyboxProceduralParameters : InspectorStruct
    {

        float turbidity = 3.0f;
        Color groundAlbedo{77, 77, 77, 255};
        Color skyTint = WHITE;
        float skyEnergy = 1.0f;


        float sunSize = 0.27f;
        float sunIntensity = 20.0f;

        Vector3 sunDirection{0.0f, -1.0f, 0.0f};
        Color sunColor = WHITE;
        float sunEnergy = 1.0f;

        SkyboxProceduralParameters() = default;

        ~SkyboxProceduralParameters() override = default;

    private:
        INSPECTOR_BEGIN(SkyboxProceduralParameters)
            INSPECT_FIELD(turbidity)
            INSPECT_FIELD(groundAlbedo)
            INSPECT_FIELD(skyTint)
            INSPECT_FIELD(skyEnergy)
            INSPECT_FIELD(sunSize)
            INSPECT_FIELD(sunIntensity)
        INSPECTOR_END()
    };
} // BreadEngine
