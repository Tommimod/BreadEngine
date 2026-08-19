#pragma once
#include "inspectorObject.h"

namespace BreadEngine {
    /**
     * How an equirectangular image is turned into a sky, beyond the image itself.
     *
     * It exists because "a sky in a file" is two different things. A full spherical capture
     * already holds whatever is under the horizon; a sky-only dome holds nothing there and
     * reads as pure black, which is both an ugly lower half and, through image-based lighting,
     * an environment that lights nothing from below.
     */
    struct SkyboxCubemapParameters : InspectorStruct
    {
        /**
         * Carry the horizon's own colour down over the lower half, dimmed by @ref groundAlbedo
         * - the same treatment the procedural dome gives the sky below its horizon.
         *
         * Off by default on purpose: an image that does have ground in it would have that
         * ground thrown away, and silently discarding authored data is the worse failure of
         * the two. A black lower half announces itself.
         */
        bool fillBelowHorizon = false;

        /// What the world below the horizon reflects. Only read while @ref fillBelowHorizon is.
        Color groundAlbedo{77, 77, 77, 255};

        SkyboxCubemapParameters() = default;

        ~SkyboxCubemapParameters() override = default;

        /**
         * Compares what a bake would read, and nothing else - the inspector's own change flag
         * is raised for every field of every block it shows, including the post-effects a sky
         * knows nothing about. A field added above and not added here stops triggering a
         * rebake, which shows up as an inspector edit that does nothing.
         */
        [[nodiscard]] bool operator==(const SkyboxCubemapParameters &other) const
        {
            return fillBelowHorizon == other.fillBelowHorizon &&
                   groundAlbedo.r == other.groundAlbedo.r && groundAlbedo.g == other.groundAlbedo.g &&
                   groundAlbedo.b == other.groundAlbedo.b && groundAlbedo.a == other.groundAlbedo.a;
        }

    private:
        INSPECTOR_BEGIN(SkyboxCubemapParameters)
            INSPECT_FIELD(fillBelowHorizon)
            INSPECT_FIELD_COND(groundAlbedo, [](const SkyboxCubemapParameters *s){return s->fillBelowHorizon;})
        INSPECTOR_END()
    };
} // BreadEngine
