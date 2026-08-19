#include "globalLightSystem.h"

#include "cameraDirector.h"
#include "engine.h"
#include "nodeProvider.h"
#include "transform.h"
#include "component/light.h"
#include "rendering/renderer.h"
#include "systems/core/filterOption.h"
#include "utils/colorUtils.h"

namespace BreadEngine {
    constexpr int PROCEDURAL_SKY_RESOLUTION = 1024;

    /**
     * How long the sky's inputs have to hold still before the image-based lighting is
     * precomputed from it.
     *
     * The sky itself is rebaked immediately, so dragging the sun moves the dome, the shadows
     * and the direct light together. The two ambient cubes are what waits: they cost as much
     * again as the sky does, and they feed reflections and fill light, which is where a
     * quarter second of lag goes unnoticed. The previous ones keep lighting the scene
     * meanwhile - dropping them would flip everything to the flat ambient colour and back on
     * every frame of a drag.
     */
    constexpr float AMBIENT_SETTLE_SECONDS = 0.25f;

    void GlobalLightSystem::startFrame(const float deltaTime)
    {
        auto &globalLight = Engine::getInstance().getGlobalLightSettings();
        auto &renderer = Renderer::get();

        trackSun(globalLight);

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
                if (const auto inputs = describeSky(globalLight, *camera); !_hasBaked || !isSameSky(inputs, _bakedSky))
                {
                    _hasBaked = true;
                    _bakedSky = inputs;
                    _steadySeconds = 0.0f;
                    rebuildSky(globalLight, inputs);
                }
                else
                {
                    _steadySeconds += deltaTime;
                }

                // An environment image is decoded off the render thread, so its cube is not
                // there on the frame it was asked for; and a sky being dragged is a different
                // cube every frame. Which sky the current ambient came from is the only thing
                // that says a precompute is owed - nothing raises a flag for either case.
                if (globalLight._background.sky.isValid() &&
                    _ambientSource != globalLight._background.sky &&
                    _steadySeconds >= AMBIENT_SETTLE_SECONDS &&
                    renderer.isCubemapReady(globalLight._background.sky))
                {
                    globalLight._ambient.generateFromCubemap(globalLight._background.sky);
                    _ambientSource = globalLight._background.sky;
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
        _hasBaked = false;
        _bakedSky = {};
        _steadySeconds = 0.0f;
        _ambientSource = {};
    }

    void GlobalLightSystem::trackSun(GlobalLightSettings &globalLight)
    {
        Light *light = nullptr;
        for (const auto node: NodeProvider::getAllNodes())
        {
            if (!node->getIsActive()) continue;
            if (!node->has<Light>()) continue;

            if (auto &candidate = node->get<Light>(); candidate.lightType == LightType::Directional)
            {
                light = &candidate;
                break;
            }
        }

        if (light == nullptr) return;

        auto &sky = globalLight._proceduralSkyboxSettings;
        sky.sunDirection = light->getOwner()->get<Transform>().getForward();
        sky.sunColor = light->color;
        sky.sunEnergy = light->intensity;
    }

    GlobalLightSystem::BakedSky GlobalLightSystem::describeSky(const GlobalLightSettings &globalLight, const Camera &camera)
    {
        BakedSky inputs;
        inputs.mode = camera.getBackgroundMode();
        inputs.type = globalLight._type;
        inputs.isFlat = inputs.mode == Camera::SOLID_COLOR ||
                        (inputs.type == GlobalLightSettings::Type::Cubemap && globalLight._skyboxTexture == nullptr);

        // Only the fields the branch rebuildSky will take actually reads, so a mode nobody is
        // in can never be the difference that triggers a rebuild.
        if (inputs.isFlat)
        {
            inputs.flatColor = camera.getBackgroundColor();
            return inputs;
        }

        switch (inputs.type)
        {
            case GlobalLightSettings::Type::Procedural: inputs.procedural = globalLight._proceduralSkyboxSettings;
                break;
            case GlobalLightSettings::Type::Cubemap: inputs.textureGuid = globalLight._skyboxTexture->getGuid();
                inputs.cubemap = globalLight._cubemapSkyboxSettings;
                break;
            case GlobalLightSettings::Type::Custom: break;
        }

        return inputs;
    }

    bool GlobalLightSystem::isSameSky(const BakedSky &a, const BakedSky &b)
    {
        return a.mode == b.mode && a.type == b.type && a.isFlat == b.isFlat &&
               a.textureGuid == b.textureGuid &&
               ColorUtils::IsCompare(a.flatColor, b.flatColor) &&
               a.procedural == b.procedural && a.cubemap == b.cubemap;
    }

    void GlobalLightSystem::rebuildSky(GlobalLightSettings &globalLight, const BakedSky &inputs)
    {
        globalLight._background.clearTexture();

        if (inputs.isFlat)
        {
            globalLight._background.color = inputs.flatColor;
            globalLight._ambient.color = inputs.flatColor;
        }
        else
        {
            switch (inputs.type)
            {
                case GlobalLightSettings::Type::Procedural:
                    globalLight._background.sky = Renderer::get().createProceduralSky(PROCEDURAL_SKY_RESOLUTION,
                                                                                     globalLight._proceduralSkyboxSettings);
                    break;
                case GlobalLightSettings::Type::Cubemap:
                    globalLight._background.setTexture(globalLight._skyboxTexture, globalLight._cubemapSkyboxSettings);
                    break;
                case GlobalLightSettings::Type::Custom: break;
            }
        }

        // The ambient map is deliberately kept where a sky was rebuilt: this runs on every
        // frame of a drag, and dropping it would flip the scene to the flat ambient colour and
        // back on each one. startFrame replaces it once the inputs settle. With no sky left to
        // replace it from, there is nothing to keep it for.
        if (!globalLight._background.sky.isValid())
        {
            globalLight._ambient.clear();
            _ambientSource = {};
        }
    }
} // namespace BreadEngine
