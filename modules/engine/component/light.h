#pragma once
#include <r3d.h>
#include "core/component.h"
#include "data/color.h"
#include "data/lightSettings.h"

namespace BreadEngine {
    struct Light final : Component
    {
        Light() = default;

        explicit Light(Node *owner);

        ~Light() override = default;

        void setLightType(R3D_LightType type);

        [[nodiscard]] R3D_LightType getLightType() const;

        void setColor(const Color &color);

        [[nodiscard]] Color getColor() const;

        void setWithShadows(bool withShadows);

    private:
        friend class LightSystem;
        R3D_Light _nativeLight = {};
        LightSettings _settings;

        INSPECTOR_BEGIN(Light)
            INSPECT_FIELD(_settings);
        INSPECTOR_END()
    };
} // BreadEngine
