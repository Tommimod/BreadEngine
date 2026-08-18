#pragma once
#include <array>

#include "raylib.h"

namespace BreadEngine {
    /**
     * Hosek-Wilkie's analytic sky dome, cooked for one set of atmospheric conditions.
     *
     * The model comes apart in two halves, and that is what makes it cheap: nine coefficients
     * per channel that depend only on the haze, the ground and how high the sun stands, and a
     * closed-form expression in those coefficients that depends on nothing but the direction
     * being looked at. Cooking is done here, once per change; the direction half is evaluated
     * per texel on the GPU.
     */
    struct HosekWilkieSky
    {
        /// The model's nine coefficients, one set per colour channel.
        std::array<std::array<float, 9>, 3> coefficients{};
        /// Absolute radiance the coefficients are scaled by, per channel.
        std::array<float, 3> radiance{};
    };

    /**
     * @param turbidity haze in the atmosphere. The dataset is tabulated from 1 (an unnaturally
     *        clear sky) to 10 (thick haze) and the value is clamped into that range.
     * @param groundAlbedo what the ground bounces back up into the sky, per channel.
     * @param sunElevation how far the sun stands above the horizon, in radians. The model has
     *        no solution below the horizon, so it is clamped there.
     */
    [[nodiscard]] HosekWilkieSky cookHosekWilkieSky(float turbidity, Vector3 groundAlbedo, float sunElevation);
} // namespace BreadEngine
