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

        /**
         * Compares what a bake would read, the sun included - it is written into the three
         * fields below from the scene's light every frame, so it belongs to the conditions as
         * much as the authored ones do. The inspector's own change flag is no substitute: it
         * is raised for every field of every block it shows, including the post-effects a sky
         * knows nothing about. A field added above and not added here stops triggering a
         * rebake, which shows up as an inspector edit that does nothing.
         */
        [[nodiscard]] bool operator==(const SkyboxProceduralParameters &other) const
        {
            return turbidity == other.turbidity && skyEnergy == other.skyEnergy &&
                   sunSize == other.sunSize && sunIntensity == other.sunIntensity &&
                   sunEnergy == other.sunEnergy &&
                   isSameColor(groundAlbedo, other.groundAlbedo) && isSameColor(skyTint, other.skyTint) &&
                   isSameColor(sunColor, other.sunColor) &&
                   sunDirection.x == other.sunDirection.x && sunDirection.y == other.sunDirection.y &&
                   sunDirection.z == other.sunDirection.z;
        }

    private:
        [[nodiscard]] static bool isSameColor(const Color a, const Color b)
        {
            return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
        }

    public:

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
