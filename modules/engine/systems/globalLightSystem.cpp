#include "globalLightSystem.h"

#include "cameraDirector.h"
#include "engine.h"
#include "utils/colorUtils.h"

namespace BreadEngine {
    void GlobalLightSystem::startFrame(const std::vector<Node *> &nodes, const float deltaTime)
    {
        auto &globalLight = Engine::getInstance().getGlobalLightSettings();
        const auto isFirstCall = globalLight._nativeEnvironment == nullptr;
        if (isFirstCall)
        {
            globalLight._nativeEnvironment = R3D_GetEnvironment();
            R3D_SetEnvironment(globalLight._nativeEnvironment);
            globalLight._environmentBackgroud = globalLight._environmentBackgroud.fromNative(globalLight._nativeEnvironment->background);
            globalLight._environmentAmbient = globalLight._environmentAmbient.fromNative(globalLight._nativeEnvironment->ambient);
        }

        for (const auto node: nodes)
        {
            const auto camera = node->get<CameraDirector>().getActiveCamera();
            if (!camera)
            {
                continue;
            }

            const auto mode = camera->getBackgroundMode();
            const auto isSolidColorUpdate = mode == Camera::SOLID_COLOR && !ColorUtils::IsCompare(globalLight._nativeEnvironment->background.color, camera->getBackgroundColor());
            if (!globalLight.isChangedFromEditor && !isFirstCall && !isSolidColorUpdate) return;
            globalLight.isChangedFromEditor = false;

            if (mode == Camera::SOLID_COLOR || (globalLight._type == GlobalLightSettings::Type::Cubemap && globalLight._skyboxTexture == nullptr))
            {
                const auto color = camera->getBackgroundColor();
                globalLight._nativeEnvironment->background.color = color;
                globalLight._nativeEnvironment->ambient.color = color;
                globalLight._nativeEnvironment->background.sky = R3D_Cubemap{};
                globalLight._nativeEnvironment->ambient.map = R3D_AmbientMap{};
            }
            else //SKYBOX
            {
                switch (globalLight._type)
                {
                    case GlobalLightSettings::Type::Procedural:
                    {
                        updateProceduralSkybox(globalLight);
                        break;
                    }
                    case GlobalLightSettings::Type::Cubemap:
                    {
                        updateCubemapSkybox(globalLight);
                        break;
                    }
                    case GlobalLightSettings::Type::Custom:
                    {
                        updateCustomSkybox(globalLight);
                        break;
                    }
                }

                globalLight._nativeEnvironment->background.color = WHITE;
                globalLight._nativeEnvironment->ambient.color = WHITE;
                globalLight._nativeEnvironment->background = globalLight._environmentBackgroud.toNative();
                globalLight._nativeEnvironment->ambient = globalLight._environmentAmbient.toNative();
            }

            globalLight._nativeEnvironment->tonemap.mode = globalLight._tonemap;
        }
    }

    void GlobalLightSystem::onDispose(const std::vector<Node *> &nodes, float deltaTime)
    {
        auto &globalLight = Engine::getInstance().getGlobalLightSettings();

        globalLight._environmentBackgroud.clearTexture();
        globalLight._environmentAmbient.clear();

        delete globalLight._nativeEnvironment;
    }

    void GlobalLightSystem::updateProceduralSkybox(GlobalLightSettings &globalLight)
    {
        if (!globalLight._proceduralSkyboxSettings.isCreated)
        {
            globalLight._proceduralSkyboxSettings = SkyboxProceduralParameters::defaultParameters();
        }
        else
        {
            globalLight._environmentBackgroud.clearTexture();
            globalLight._environmentAmbient.clear();
        }

        globalLight._nativeProceduralSky = R3D_ProceduralSky{
            .skyTopColor = globalLight._proceduralSkyboxSettings.skyTopColor,
            .skyHorizonColor = globalLight._proceduralSkyboxSettings.skyHorizonColor,
            .skyHorizonCurve = globalLight._proceduralSkyboxSettings.skyHorizonCurve,
            .skyEnergy = globalLight._proceduralSkyboxSettings.skyEnergy,
            .groundBottomColor = globalLight._proceduralSkyboxSettings.groundBottomColor,
            .groundHorizonColor = globalLight._proceduralSkyboxSettings.groundHorizonColor,
            .groundHorizonCurve = globalLight._proceduralSkyboxSettings.groundHorizonCurve,
            .groundEnergy = globalLight._proceduralSkyboxSettings.groundEnergy,
            .sunDirection = globalLight._proceduralSkyboxSettings.sunDirection,
            .sunColor = globalLight._proceduralSkyboxSettings.sunColor,
            .sunSize = globalLight._proceduralSkyboxSettings.sunSize,
            .sunEnergy = globalLight._proceduralSkyboxSettings.sunEnergy
        };
        globalLight._environmentBackgroud.sky = R3D_GenProceduralSky(1024, globalLight._nativeProceduralSky);
        globalLight._environmentAmbient.generateFromCubemap(globalLight._environmentBackgroud.sky);
    }

    void GlobalLightSystem::updateCubemapSkybox(GlobalLightSettings &globalLight)
    {
    }

    void GlobalLightSystem::updateCustomSkybox(GlobalLightSettings &globalLight)
    {
    }
} // BreadEngine
