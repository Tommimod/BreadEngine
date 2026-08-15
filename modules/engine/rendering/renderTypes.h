#pragma once
#include <cstdint>
#include <string>

#include "raylib.h"
#include "renderHandles.h"

namespace BreadEngine {
    /**
     * Enums below are serialized by *declaration index* (magic_enum::enum_index, see
     * inspectorObject.h) - not by name, not by underlying value. Reordering or inserting a
     * value silently changes what already-saved .nd / .cnf files deserialize to, so append
     * only.
     */

    enum class LightType : uint8_t
    {
        Directional = 0,
        Spot,
        Omni
    };

    enum class TextureFilterMode : uint8_t
    {
        Point = 0,
        Bilinear,
        Trilinear,
        Anisotropic4x,
        Anisotropic8x,
        Anisotropic16x
    };

    enum class TextureWrapMode : uint8_t
    {
        Repeat = 0,
        Clamp,
        MirrorRepeat,
        MirrorClamp
    };

    /**
     * Desired state of one light, pushed to the renderer every frame; the renderer diffs it
     * against what it has already applied.
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

    struct TextureDesc
    {
        std::string path;
        TextureFilterMode filter = TextureFilterMode::Point;
        TextureWrapMode wrap = TextureWrapMode::Repeat;
        /// Sample through the sRGB color space. False for data maps (normal, ORM).
        bool isColor = true;
    };

    struct TextureSize
    {
        int width = 0;
        int height = 0;
    };

    /// PBR texture set of a surface. An invalid handle leaves the renderer's default in place.
    struct MaterialData
    {
        TextureHandle albedo;
        TextureHandle normal;
        TextureHandle orm;
        TextureHandle emission;
    };
} // namespace BreadEngine
