#include "hosekWilkie.h"

#include <algorithm>
#include <cmath>
#include <numbers>

// The reference implementation's coefficient tables, verbatim, under the BSD licence next to
// them. They declare plain globals rather than anything guarded, so this is the one
// translation unit allowed to include them.
#include "hosekWilkieData.inl"

namespace BreadEngine {
    namespace {
        /// Coefficients the model defines per channel. The radiance table has one value where
        /// the main table has nine, and is otherwise laid out identically.
        constexpr int SKY_COEFFICIENT_COUNT = 9;

        /// Control points the elevation interpolation spends on each coefficient.
        constexpr int CONTROL_POINT_COUNT = 6;

        /// Turbidity steps the dataset is tabulated at, and the two ground-albedo extremes it
        /// is tabulated for. Both are interpolated between rather than picked.
        constexpr int TURBIDITY_LEVELS = 10;
        constexpr int ALBEDO_LEVELS = 2;

        /// One quintic Bezier over the six control points found @p stride apart from @p from.
        /// The dataset stores a coefficient's control points interleaved with its neighbours',
        /// which is why the step between them is not one.
        [[nodiscard]] double interpolateElevation(const double *from, const int stride, const double elevation)
        {
            const double t = elevation;
            const double s = 1.0 - t;
            return s * s * s * s * s * from[0]
                   + 5.0 * s * s * s * s * t * from[stride]
                   + 10.0 * s * s * s * t * t * from[stride * 2]
                   + 10.0 * s * s * t * t * t * from[stride * 3]
                   + 5.0 * s * t * t * t * t * from[stride * 4]
                   + t * t * t * t * t * from[stride * 5];
        }

        /**
         * Blends one channel's table down to @p count coefficients for the given conditions:
         * a Bezier in elevation, then linearly between the two turbidity steps it falls
         * between and the two albedo extremes.
         */
        void cookChannel(const double *dataset, const int count, const double turbidity,
                         const double albedo, const double elevation, float *out)
        {
            const int blockSize = count * CONTROL_POINT_COUNT;
            const int albedoStride = blockSize * TURBIDITY_LEVELS;

            const int lowLevel = std::clamp(static_cast<int>(turbidity), 1, TURBIDITY_LEVELS);
            const double levelFraction = std::clamp(turbidity - lowLevel, 0.0, 1.0);
            // The tables are tabulated against the cube root of the elevation normalized to
            // the zenith, which is what spends their resolution where the sky changes fastest.
            const double zenithFraction = std::clamp(elevation / (std::numbers::pi / 2.0), 0.0, 1.0);
            const double curvedElevation = std::cbrt(zenithFraction);

            for (int index = 0; index < count; ++index)
            {
                double value = 0.0;
                for (int albedoLevel = 0; albedoLevel < ALBEDO_LEVELS; ++albedoLevel)
                {
                    const double albedoWeight = albedoLevel == 0 ? 1.0 - albedo : albedo;
                    for (int levelStep = 0; levelStep < 2; ++levelStep)
                    {
                        const double levelWeight = levelStep == 0 ? 1.0 - levelFraction : levelFraction;
                        if (levelWeight == 0.0) continue;

                        // The top of the range has no step above it to blend towards, and
                        // levelFraction is zero there, so clamping costs nothing.
                        const int level = std::min(lowLevel - 1 + levelStep, TURBIDITY_LEVELS - 1);
                        const double *block = dataset + albedoLevel * albedoStride + level * blockSize + index;
                        value += albedoWeight * levelWeight * interpolateElevation(block, count, curvedElevation);
                    }
                }

                out[index] = static_cast<float>(value);
            }
        }
    } // namespace

    HosekWilkieSky cookHosekWilkieSky(const float turbidity, const Vector3 groundAlbedo, const float sunElevation)
    {
        const double clampedTurbidity = std::clamp(static_cast<double>(turbidity), 1.0, static_cast<double>(TURBIDITY_LEVELS));
        const double elevation = std::max(static_cast<double>(sunElevation), 0.0);
        const double albedo[3]{groundAlbedo.x, groundAlbedo.y, groundAlbedo.z};

        HosekWilkieSky sky;
        for (int channel = 0; channel < 3; ++channel)
        {
            const double channelAlbedo = std::clamp(albedo[channel], 0.0, 1.0);
            cookChannel(datasetsRGB[channel], SKY_COEFFICIENT_COUNT, clampedTurbidity, channelAlbedo, elevation,
                        sky.coefficients[channel].data());
            cookChannel(datasetsRGBRad[channel], 1, clampedTurbidity, channelAlbedo, elevation,
                        &sky.radiance[channel]);
        }

        return sky;
    }
} // namespace BreadEngine
