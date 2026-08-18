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
        struct BakedSun
        {
            Vector3 direction{};
            Color color{};
            float energy = 0.0f;
        };

        bool _hasStarted = false;
        BakedSun _bakedSun{};

        static void updateProceduralSkybox(GlobalLightSettings &globalLight);

        static void updateCubemapSkybox(GlobalLightSettings &globalLight);

        static void updateCustomSkybox(GlobalLightSettings &globalLight);

        [[nodiscard]] bool trackProceduralSun(GlobalLightSettings &globalLight);
    };
} // namespace BreadEngine
