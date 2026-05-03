#pragma once
#include <r3d.h>
#include "core/component.h"
#include "data/color.h"

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

        void update(float deltaTime) override;

        void setWithShadows(bool withShadows) const;

        void setShadowResolution(int resolution);

    private:
        R3D_Light _nativeLight{};
        R3D_LightType _lightType = R3D_LIGHT_DIR;
        R3D_LightType _prevLightType = R3D_LIGHT_DIR;
        Color _color;
        Color _prevColor;
        bool _withShadows = true;
        bool _prevWithShadows = true;
        int _shadowResolution = 512;

        INSPECTOR_BEGIN(Light)
            INSPECT_FIELD(_lightType);
            INSPECT_FIELD(_withShadows);
            INSPECT_FIELD(_color);
        INSPECTOR_END()
    };
} // BreadEngine
