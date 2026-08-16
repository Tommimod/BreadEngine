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

    /**
     * How a rendered frame leaves the renderer. Shading always happens in linear space; this
     * only decides whether the result is encoded to gamma space on its way to the target;
     * Linear writes it out untouched.
     */
    enum class OutputColorSpace : uint8_t
    {
        Gamma = 0,
        Linear
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

    enum class ProjectionType : uint8_t
    {
        Perspective = 0,
        Orthographic
    };

    enum class BloomMode : uint8_t
    {
        Disabled = 0,
        Mix,
        Additive,
        Screen
    };

    enum class FogMode : uint8_t
    {
        Disabled = 0,
        Linear,
        Exp2,
        Exp
    };

    enum class DepthOfFieldMode : uint8_t
    {
        Disabled = 0,
        Enabled
    };

    enum class TonemapMode : uint8_t
    {
        Linear = 0,
        Reinhard,
        Filmic,
        Aces,
        Agx
    };

    /// Viewpoint the scene is rendered from.
    struct CameraView
    {
        Vector3 position{};
        Vector3 target{};
        Vector3 up{0.0f, 1.0f, 0.0f};
        float fov = 45.0f;
        ProjectionType projection = ProjectionType::Perspective;
    };

    /// Mapped explicitly rather than cast: ProjectionType's numbering is a serialization
    /// contract and has to stay free to diverge from raylib's own enum.
    [[nodiscard]] inline int toRaylibProjection(const ProjectionType projection)
    {
        return projection == ProjectionType::Orthographic ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;
    }

    /// For the call sites that still drive the camera through raylib: the editor viewport
    /// and the game loop.
    [[nodiscard]] inline CameraView toCameraView(const Camera3D &camera)
    {
        return CameraView{
            .position = camera.position,
            .target = camera.target,
            .up = camera.up,
            .fov = camera.fovy,
            .projection = camera.projection == CAMERA_ORTHOGRAPHIC ? ProjectionType::Orthographic : ProjectionType::Perspective
        };
    }

    /// PBR texture set of a surface. An invalid handle leaves the renderer's default in place.
    struct MaterialData
    {
        TextureHandle albedo;
        TextureHandle normal;
        TextureHandle orm;
        TextureHandle emission;
    };
} // namespace BreadEngine
