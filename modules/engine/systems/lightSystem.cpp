#include "lightSystem.h"

#include "transform.h"
#include "component/light.h"

namespace BreadEngine {
    void LightSystem::initialize(const std::vector<Node *> &nodes)
    {
        for (const auto node: nodes)
        {
            if (!node->has<Light>()) continue;

            auto &light = node->get<Light>();
            light.setLightType(light._lightType);
            light.setWithShadows(light._withShadows);
            light.setShadowResolution(light._shadowResolution);
            light.setColor(light._color);
        }
    }

    void LightSystem::update(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<Light>()) continue;

            auto &light = node->get<Light>();
            if (light._prevLightType != light._lightType)
            {
                light.setLightType(light._lightType);
                light._prevLightType = light._lightType;
            }

            if (light._prevWithShadows != light._withShadows)
            {
                light._prevWithShadows = light._withShadows;
                light.setWithShadows(light._withShadows);
            }

            if (light._prevColor != light._color)
            {
                light._prevColor = light._color;
                light.setColor(light._color);
            }

            if (const auto isActive = R3D_IsLightActive(light._nativeLight); isActive != node->getIsActive())
            {
                R3D_SetLightActive(light._nativeLight, node->getIsActive());
            }

            if (light._lightType == R3D_LIGHT_DIR) continue;
            const auto &transform = node->get<Transform>();
            const auto pos = transform.getPosition();
            const auto direction = transform.getForward();
            R3D_SetLightPosition(light._nativeLight, pos);
            R3D_SetLightDirection(light._nativeLight, direction);
        }
    }
} // BreadEngine
