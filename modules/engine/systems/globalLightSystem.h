#pragma once
// Not forward declared: BakedSky stores a Camera::BackgroundMode by value.
#include "component/camera.h"
#include "configs/light/globalLightSettings.h"
#include "core/standaloneDisposeSystem.h"
#include "core/standaloneStartFrameSystem.h"

namespace BreadEngine {
    class GlobalLightSystem final : public StandaloneStartFrameSystem<GlobalLightSystem>, public StandaloneDisposeSystem<GlobalLightSystem>
    {
    public:
        void startFrame(float deltaTime) override;

        void onDispose(float deltaTime) override;

    private:
        /**
         * Everything a baked sky is built from, as it stood when it was last built.
         *
         * The sky is rebuilt by comparing against this rather than by watching
         * `isChangedFromEditor`, because that flag belongs to the whole of
         * `GlobalLightSettings`: toggling a post-effect the sky knows nothing about raises it
         * just as an edit to the turbidity does, and a rebuild costs an environment image
         * being decoded from disk.
         *
         * Only the fields the taken branch actually reads are filled, so a mode nobody is in
         * can never be the reason for a rebuild.
         */
        struct BakedSky
        {
            Camera::BackgroundMode mode = Camera::SOLID_COLOR;
            GlobalLightSettings::Type type = GlobalLightSettings::Type::Procedural;
            /// Which of the two shapes below was built. Recorded rather than re-derived: it is
            /// the one thing the rebuild has to agree with the snapshot about, and deciding it
            /// twice is how the two would come to disagree.
            bool isFlat = false;
            /// The camera's own colour, for the branch that has no sky at all.
            Color flatColor{};
            /// The image, by guid rather than by address: assets are rebuilt when a project
            /// reloads, and a pointer to a freed one can compare equal to a different asset
            /// that lands at the same place.
            std::string textureGuid;
            SkyboxProceduralParameters procedural{};
            SkyboxCubemapParameters cubemap{};
        };

        bool _hasBaked = false;
        BakedSky _bakedSky{};
        /// How long the sky's inputs have held still. The image-based lighting waits for this
        /// where the sky itself does not - see AMBIENT_SETTLE_SECONDS.
        float _steadySeconds = 0.0f;
        /// The sky the current ambient map was precomputed from. Compared rather than flagged,
        /// because a rebuilt sky raises nothing and a loaded one arrives frames late.
        CubemapHandle _ambientSource{};

        /// Copies the scene's directional light into the procedural sky's sun fields, so the
        /// sky and the light that casts the shadows cannot disagree about where the sun is.
        /// Runs whatever the sky type is - the fields are free, and the comparison below is
        /// what decides whether any of it matters.
        static void trackSun(GlobalLightSettings &globalLight);

        [[nodiscard]] static BakedSky describeSky(const GlobalLightSettings &globalLight, const Camera &camera);

        [[nodiscard]] static bool isSameSky(const BakedSky &a, const BakedSky &b);

        /// Releases the sky and starts whatever @p inputs describe. The ambient map outlives
        /// this on purpose; startFrame replaces it once the inputs settle.
        void rebuildSky(GlobalLightSettings &globalLight, const BakedSky &inputs);
    };
} // namespace BreadEngine
