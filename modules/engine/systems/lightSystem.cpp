#include "lightSystem.h"

#include "transform.h"
#include "component/light.h"
#include "utils/colorUtils.h"

namespace BreadEngine {
    void LightSystem::update(const std::vector<Node *> &nodes, float deltaTime)
    {
        for (const auto node: nodes)
        {
            if (!node->has<Light>()) continue;

            auto &light = node->get<Light>();
            if (!R3D_IsLightExist(light._nativeLight) || R3D_GetLightType(light._nativeLight) != light.lightType)
            {
                if (R3D_IsLightExist(light._nativeLight))
                {
                    R3D_DestroyLight(light._nativeLight);
                }

                light._nativeLight = R3D_CreateLight(light.lightType);
            }

            if (light.withShadows && !R3D_IsShadowEnabled(light._nativeLight))
            {
                R3D_EnableShadow(light._nativeLight);
            }
            else if (!light.withShadows && R3D_IsShadowEnabled(light._nativeLight))
            {
                R3D_DisableShadow(light._nativeLight);
            }

            if (!ColorUtils::IsCompare(R3D_GetLightColor(light._nativeLight), light.color))
            {
                R3D_SetLightColor(light._nativeLight, light.color);
            }

            if (R3D_GetLightRange(light._nativeLight) != light.range)
            {
                R3D_SetLightRange(light._nativeLight, light.range);
            }

            if (light.withShadows && R3D_GetShadowSoftness(light._nativeLight) != light.shadowSoftness)
            {
                R3D_SetShadowSoftness(light._nativeLight, light.shadowSoftness);
            }

            if (R3D_GetLightEnergy(light._nativeLight) != light.intensity)
            {
                R3D_SetLightEnergy(light._nativeLight, light.intensity);
            }

            if (const auto isActive = R3D_IsLightActive(light._nativeLight); isActive != node->getIsActive())
            {
                R3D_SetLightActive(light._nativeLight, node->getIsActive());
            }

            const auto &transform = node->get<Transform>();
            if (light.lightType != R3D_LIGHT_OMNI)
            {
                const auto direction = transform.getForward();
                R3D_SetLightDirection(light._nativeLight, direction);
            }

            if (light.lightType == R3D_LIGHT_DIR) continue;
            const auto pos = transform.getPosition();
            R3D_SetLightPosition(light._nativeLight, pos);
        }
    }
} // BreadEngine
