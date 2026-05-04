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
            light.setLightType(light._settings.lightType);
            light.setWithShadows(light._settings.withShadows);
            light.setColor(light._settings.color);
        }
    }

    void LightSystem::update(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<Light>()) continue;

            auto &light = node->get<Light>();
            light.setLightType(light._settings.lightType);
            light.setWithShadows(light._settings.withShadows);
            light.setColor(light._settings.color);

            if (const auto isActive = R3D_IsLightActive(light._nativeLight); isActive != node->getIsActive())
            {
                R3D_SetLightActive(light._nativeLight, node->getIsActive());
            }

            if (light._settings.lightType == R3D_LIGHT_DIR) continue;
            const auto &transform = node->get<Transform>();
            const auto pos = transform.getPosition();
            const auto direction = transform.getForward();
            R3D_SetLightPosition(light._nativeLight, pos);
            R3D_SetLightDirection(light._nativeLight, direction);
        }
    }
} // BreadEngine
