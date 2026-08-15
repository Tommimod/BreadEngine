#pragma once
#include <cstdint>
#include "raylib.h"

namespace BreadEngine {
    /**
     * Engine-native replacement for R3D_LightType.
     *
     * IMPORTANT - do not reorder: the inspector serializes enums by *declaration index*
     * (magic_enum::enum_index, see inspectorObject.h), not by name and not by underlying
     * value. Directional/Spot/Omni therefore has to stay in R3D_LightType's original
     * DIR/SPOT/OMNI order so already-saved .nd scene files keep deserializing to the same
     * light type they had before the seam went in. (R3D_LIGHT_TYPE_COUNT was index 3 and
     * used to show up as a selectable entry in the inspector's dropdown - dropping it is
     * safe precisely because it was last.)
     */
    enum class LightType : uint8_t
    {
        Directional = 0,
        Spot,
        Omni
    };

    /**
     * The complete desired state of one light, pushed to the renderer each frame.
     *
     * The backend - not the caller - owns diffing this against what it has already applied.
     * A light is an object with setters in r3d, but under Diligent it will be one entry in a
     * lights constant buffer plus a shadow-map slot, so the seam describes *what the light
     * is* rather than mirroring r3d's setter-by-setter API.
     */
    struct LightState
    {
        LightType type = LightType::Directional;
        Color color = WHITE;
        Vector3 position{};
        Vector3 direction{};
        float range = 50.0f;
        float intensity = 1.0f;
        float shadowSoftness = 1.0f;
        bool castShadows = true;
        bool active = true;
    };
} // namespace BreadEngine
