#pragma once
#include "configs/light/globalLightSettings.h"
#include "core/disposeSystem.h"
#include "core/startFrameSystem.h"

namespace BreadEngine {
    class GlobalLightSystem : public StartFrameSystem, public DisposeSystem
    {
    public:
        void startFrame(Node *node, float deltaTime) override;

        void onDispose(Node *node, float deltaTime) override;

    private:
        static void updateProceduralSkybox(GlobalLightSettings &globalLight);

        static void updateCubemapSkybox(GlobalLightSettings &globalLight);

        static void updateCustomSkybox(GlobalLightSettings &globalLight);

        static void updateProceduralSunPosition(GlobalLightSettings &globalLight);
    };
} // BreadEngine
