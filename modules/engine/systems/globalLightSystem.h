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
        /// The background and its ambient map are built on the first frame regardless of what
        /// changed; cleared on dispose so leaving play mode rebuilds them.
        bool _hasStarted = false;

        static void updateProceduralSkybox(GlobalLightSettings &globalLight);
        static void updateCubemapSkybox(GlobalLightSettings &globalLight);
        static void updateCustomSkybox(GlobalLightSettings &globalLight);
        static void updateProceduralSunPosition(GlobalLightSettings &globalLight);
    };
} // namespace BreadEngine
