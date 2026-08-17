#pragma once
#include "raylib.h"
#include "core/component.h"
#include "rendering/renderHandles.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct Light final : Component
    {
        LightType lightType = LightType::Directional;
        Color color = WHITE;
        float range = 50;
        float intensity = 1;
        float spotAngle = 45;
        float spotBlend = 0.15f;
        float shadowSoftness = 1;
        bool withShadows = true;

        Light() = default;

        explicit Light(Node *owner);

        ~Light() override = default;

        Light(const Light &other);

        Light &operator=(const Light &other);

        Light(Light &&other) noexcept;

        Light &operator=(Light &&other) noexcept;

        void onDestroy() override;

    private:
        friend class LightSystem;
        LightHandle _handle{};

        void copySettings(const Light &other);

        void releaseHandle();

        INSPECTOR_BEGIN(Light)
            INSPECT_FIELD(lightType);
            INSPECT_FIELD(withShadows);
            INSPECT_FIELD_COND(shadowSoftness, [](const Light* l){return l->withShadows;});
            INSPECT_FIELD(color);
            INSPECT_FIELD_COND(range, [](const Light *l){return l->lightType != LightType::Directional;});
            INSPECT_FIELD_COND(spotAngle, [](const Light *l){return l->lightType == LightType::Spot;});
            INSPECT_FIELD_COND(spotBlend, [](const Light *l){return l->lightType == LightType::Spot;});
            INSPECT_FIELD(intensity);
        INSPECTOR_END()
    };
} // BreadEngine
