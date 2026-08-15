#pragma once
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
        /// The renderer seeds the environment with its own values once per session; cleared
        /// on dispose so leaving play mode re-seeds.
        bool _seeded = false;

        static void updateProceduralSkybox(GlobalLightSettings &globalLight);
        static void updateCubemapSkybox(GlobalLightSettings &globalLight);
        static void updateCustomSkybox(GlobalLightSettings &globalLight);
        static void updateProceduralSunPosition(GlobalLightSettings &globalLight);
    };
} // namespace BreadEngine
