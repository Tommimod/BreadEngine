#include "lightSystem.h"

#include "transform.h"
#include "component/light.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    void LightSystem::update(Node *node, float deltaTime)
    {
        if (!node->getIsActive()) return;
        if (!node->has<Light>()) return;

        auto &light = node->get<Light>();
        auto &renderer = Renderer::get();
        if (!renderer.isLightValid(light._handle))
        {
            light._handle = renderer.createLight(light.lightType);
        }

        const auto &transform = node->get<Transform>();
        renderer.updateLight(light._handle, LightState{
                                 .type = light.lightType,
                                 .color = light.color,
                                 .position = transform.getPosition(),
                                 .direction = transform.getForward(),
                                 .range = light.range,
                                 .intensity = light.intensity,
                                 .shadowSoftness = light.shadowSoftness,
                                 .castShadows = light.withShadows,
                                 .active = node->getIsActive()
                             });
    }
} // namespace BreadEngine
