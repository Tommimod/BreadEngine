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
        float shadowSoftness = 1;
        bool withShadows = true;

        Light() = default;

        explicit Light(Node *owner);

        ~Light() override = default;

    private:
        friend class LightSystem;
        LightHandle _handle{};

        INSPECTOR_BEGIN(Light)
            INSPECT_FIELD(lightType);
            INSPECT_FIELD(withShadows);
            INSPECT_FIELD_COND(shadowSoftness, [](const Light* l){return l->withShadows;});
            INSPECT_FIELD(color);
            INSPECT_FIELD(range);
            INSPECT_FIELD(intensity);
        INSPECTOR_END()
    };
} // BreadEngine
