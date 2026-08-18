#include "globalLightSystem.h"

#include "cameraDirector.h"
#include "engine.h"
#include "nodeProvider.h"
#include "transform.h"
#include "component/light.h"
#include "raymath.h"
#include "rendering/renderer.h"
#include "systems/core/filterOption.h"
#include "utils/colorUtils.h"

namespace BreadEngine {
    constexpr int PROCEDURAL_SKY_RESOLUTION = 1024;

    void GlobalLightSystem::startFrame(const float deltaTime)
    {
        auto &globalLight = Engine::getInstance().getGlobalLightSettings();
        auto &renderer = Renderer::get();

        const bool isFirstCall = !_hasStarted;
        _hasStarted = true;

        const bool hasSunMoved = trackProceduralSun(globalLight);

        static const FilterOption kCameraDirectorFilter = FilterOption::empty().with<CameraDirector>();
        Node *cameraDirectorNode = nullptr;
        for (const auto node: NodeProvider::getAllNodes())
        {
            if (kCameraDirectorFilter.isValid(*node))
            {
                cameraDirectorNode = node;
                break;
            }
        }

        if (cameraDirectorNode != nullptr)
        {
            const auto camera = cameraDirectorNode->get<CameraDirector>().getActiveCamera();
            if (camera != nullptr)
            {
                const auto mode = camera->getBackgroundMode();
                const bool isSolidColorUpdate = mode == Camera::SOLID_COLOR &&
                                                !ColorUtils::IsCompare(globalLight._background.color, camera->getBackgroundColor());

                if (globalLight.isChangedFromEditor || isFirstCall || isSolidColorUpdate || hasSunMoved)
                {
                    globalLight.isChangedFromEditor = false;

                    if (mode == Camera::SOLID_COLOR ||
                        (globalLight._type == GlobalLightSettings::Type::Cubemap && globalLight._skyboxTexture == nullptr))
                    {
                        const auto color = camera->getBackgroundColor();
                        globalLight._background.clearTexture();
                        globalLight._ambient.clear();
                        globalLight._background.color = color;
                        globalLight._ambient.color = color;
                    }
                    else
                    {
                        switch (globalLight._type)
                        {
                            case GlobalLightSettings::Type::Procedural: updateProceduralSkybox(globalLight);
                                break;
                            case GlobalLightSettings::Type::Cubemap: updateCubemapSkybox(globalLight);
                                break;
                            case GlobalLightSettings::Type::Custom: updateCustomSkybox(globalLight);
                                break;
                        }
                    }
                }
            }
        }

        renderer.setEnvironment(globalLight.environment());
    }

    void GlobalLightSystem::onDispose(const float deltaTime)
    {
        auto &globalLight = Engine::getInstance().getGlobalLightSettings();
        globalLight._background.clearTexture();
        globalLight._ambient.clear();
        _hasStarted = false;
        _bakedSun = {};
    }

    void GlobalLightSystem::updateProceduralSkybox(GlobalLightSettings &globalLight)
    {
        globalLight._background.clearTexture();
        globalLight._background.sky = Renderer::get().createProceduralSky(PROCEDURAL_SKY_RESOLUTION, globalLight._proceduralSkyboxSettings);
        globalLight._ambient.generateFromCubemap(globalLight._background.sky);
    }

    void GlobalLightSystem::updateCubemapSkybox(GlobalLightSettings &globalLight)
    {
        globalLight._background.setTexture(globalLight._skyboxTexture);
        globalLight._ambient.generateFromCubemap(globalLight._background.sky);
    }

    void GlobalLightSystem::updateCustomSkybox(GlobalLightSettings &globalLight)
    {
    }

    bool GlobalLightSystem::trackProceduralSun(GlobalLightSettings &globalLight)
    {
        auto &nodes = NodeProvider::getAllNodes();
        Light *light = nullptr;
        for (const auto n: nodes)
        {
            if (!n->getIsActive()) continue;
            if (n->has<Light>())
            {
                auto &l = n->get<Light>();
                if (l.lightType == LightType::Directional)
                {
                    light = &l;
                    break;
                }
            }
        }

        if (light == nullptr) return false;

        auto &sky = globalLight._proceduralSkyboxSettings;
        sky.sunDirection = light->getOwner()->get<Transform>().getForward();
        sky.sunColor = light->color;
        sky.sunEnergy = light->intensity;


        if (globalLight._type != GlobalLightSettings::Type::Procedural) return false;

        const bool hasMoved = !Vector3Equals(sky.sunDirection, _bakedSun.direction) ||
                              !ColorUtils::IsCompare(sky.sunColor, _bakedSun.color) ||
                              sky.sunEnergy != _bakedSun.energy;
        if (!hasMoved) return false;

        _bakedSun = BakedSun{.direction = sky.sunDirection, .color = sky.sunColor, .energy = sky.sunEnergy};
        return true;
    }
} // namespace BreadEngine
