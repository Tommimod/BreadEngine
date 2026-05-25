#include "globalLightSystem.h"

#include "cameraDirector.h"
#include "engine.h"
#include "transform.h"
#include "component/light.h"
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
            if (!globalLight.isChangedFromEditor)
            {
                globalLight._background = globalLight._background.fromNative(globalLight._nativeEnvironment->background);
                globalLight._ambient = globalLight._ambient.fromNative(globalLight._nativeEnvironment->ambient);
                globalLight._ssao = globalLight._ssao.fromNative(globalLight._nativeEnvironment->ssao);
                globalLight._ssil = globalLight._ssil.fromNative(globalLight._nativeEnvironment->ssil);
                globalLight._ssgi = globalLight._ssgi.fromNative(globalLight._nativeEnvironment->ssgi);
                globalLight._ssr = globalLight._ssr.fromNative(globalLight._nativeEnvironment->ssr);
                globalLight._bloom = globalLight._bloom.fromNative(globalLight._nativeEnvironment->bloom);
                globalLight._fog = globalLight._fog.fromNative(globalLight._nativeEnvironment->fog);
                globalLight._depthOfField = globalLight._depthOfField.fromNative(globalLight._nativeEnvironment->dof);
                globalLight._tonemap = globalLight._tonemap.fromNative(globalLight._nativeEnvironment->tonemap);
                globalLight._finalColor = globalLight._finalColor.fromNative(globalLight._nativeEnvironment->color);
            }
        }

        for (const auto node: nodes)
        {
            if (!node->has<CameraDirector>()) continue;
            const auto camera = node->get<CameraDirector>().getActiveCamera();
            if (!camera) continue;

            const auto mode = camera->getBackgroundMode();
            const auto isSolidColorUpdate = mode == Camera::SOLID_COLOR && !ColorUtils::IsCompare(globalLight._nativeEnvironment->background.color, camera->getBackgroundColor());
            if (!globalLight.isChangedFromEditor && !isFirstCall && !isSolidColorUpdate)
            {
                continue;
            }

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

                globalLight._nativeEnvironment->background = globalLight._background.toNative();
                globalLight._nativeEnvironment->ambient = globalLight._ambient.toNative();
            }

            globalLight._nativeEnvironment->ssao = globalLight._ssao.toNative();
            globalLight._nativeEnvironment->ssil = globalLight._ssil.toNative();
            globalLight._nativeEnvironment->ssgi = globalLight._ssgi.toNative();
            globalLight._nativeEnvironment->ssr = globalLight._ssr.toNative();
            globalLight._nativeEnvironment->bloom = globalLight._bloom.toNative();
            globalLight._nativeEnvironment->fog = globalLight._fog.toNative();
            globalLight._nativeEnvironment->dof = globalLight._depthOfField.toNative();
            globalLight._nativeEnvironment->tonemap = globalLight._tonemap.toNative();
            globalLight._nativeEnvironment->color = globalLight._finalColor.toNative();
        }

        updateProceduralSunPosition(globalLight);
    }

    void GlobalLightSystem::onDispose(const std::vector<Node *> &nodes, float deltaTime)
    {
        auto &globalLight = Engine::getInstance().getGlobalLightSettings();
        globalLight._background.clearTexture();
        globalLight._ambient.clear();
        globalLight._nativeEnvironment = nullptr;
    }

    void GlobalLightSystem::updateProceduralSkybox(GlobalLightSettings &globalLight)
    {
        if (!globalLight._proceduralSkyboxSettings.isCreated)
        {
            globalLight._proceduralSkyboxSettings = SkyboxProceduralParameters::defaultParameters();
        }
        else
        {
            globalLight._background.clearTexture();
            globalLight._ambient.clear();
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
        globalLight._background.sky = R3D_GenProceduralSky(1024, globalLight._nativeProceduralSky);
        globalLight._ambient.generateFromCubemap(globalLight._background.sky);
    }

    void GlobalLightSystem::updateCubemapSkybox(GlobalLightSettings &globalLight)
    {
    }

    void GlobalLightSystem::updateCustomSkybox(GlobalLightSettings &globalLight)
    {
    }

    void GlobalLightSystem::updateProceduralSunPosition(GlobalLightSettings &globalLight)
    {
        auto &nodes = NodeProvider::getAllNodes();
        Light *light = nullptr;
        for (const auto n: nodes)
        {
            if (n->has<Light>())
            {
                auto &l = n->get<Light>();
                if (l.lightType == R3D_LIGHT_DIR)
                {
                    light = &l;
                    break;
                }
            }
        }

        if (light == nullptr)
        {
            return;
        }

        const auto &transform = light->getOwner()->get<Transform>();
        if (!transform.isChangedFromEditor && !light->isChangedFromEditor)
        {
            return;
        }

        const auto forward = transform.getForward();
        globalLight._proceduralSkyboxSettings.sunDirection = forward;
        globalLight._nativeProceduralSky.sunDirection = globalLight._proceduralSkyboxSettings.sunDirection;
        globalLight._proceduralSkyboxSettings.sunColor = light->color;
        globalLight._nativeProceduralSky.sunColor = globalLight._proceduralSkyboxSettings.sunColor;
        globalLight._proceduralSkyboxSettings.sunEnergy = light->intensity;
        globalLight._nativeProceduralSky.sunEnergy = globalLight._proceduralSkyboxSettings.sunEnergy;
    }
} // BreadEngine
