#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "raylib.h"
#include "renderHandles.h"

namespace BreadEngine {
    /**
     * One vertex of overlay geometry. What the position means - a point in the world, or a
     * pixel of the target - is decided by the shader that reads it, and so is whether the
     * colour and the texture coordinate are read at all. All three are carried regardless:
     * one layout is what lets a line list, a widget rect and a glyph quad share a pipeline
     * without the renderer being told which is which.
     */
    struct OverlayVertex
    {
        Vector3 position{};
        Vector2 uv{};
        Color color{255, 255, 255, 255};
    };

    struct OverlayMeshData
    {
        std::vector<OverlayVertex> vertices;
        /// Left empty, the vertices are drawn in the order they are given.
        std::vector<uint32_t> indices;

        [[nodiscard]] bool isEmpty() const { return vertices.empty(); }
    };

    enum class OverlayTopology : uint8_t
    {
        Triangles = 0,
        Lines
    };

    enum class OverlayBlendMode : uint8_t
    {
        Opaque = 0,
        Alpha
    };

    /**
     * How a draw stands against the depth the scene left behind. Test alone is what an
     * overlay usually wants: occluded by the geometry in front of it, and still transparent
     * to whatever the client draws next.
     */
    enum class OverlayDepthMode : uint8_t
    {
        Disabled = 0,
        Test,
        TestAndWrite
    };

    /**
     * A shader pair the client ships, and the pipeline state it is drawn through. The
     * renderer compiles and owns it without knowing what it draws.
     */
    struct OverlayEffectDesc
    {
        /// Where the two sources sit, relative to the executable. The engine's own shader
        /// directory is searched as well, which is what makes overlay.fxh includable.
        std::string shaderDirectory;
        std::string vertexShader;
        std::string pixelShader;
        /// Bytes of the constant block the shaders declare of their own, or zero when they
        /// declare none. A draw's parameters are uploaded into it verbatim, so this and the
        /// cbuffer in the shader are one declaration made twice.
        uint32_t parameterSize = 0;
        OverlayTopology topology = OverlayTopology::Triangles;
        OverlayBlendMode blend = OverlayBlendMode::Alpha;
        OverlayDepthMode depth = OverlayDepthMode::Test;
        bool cullBackFaces = true;
    };

    struct OverlayDrawDesc
    {
        OverlayMeshHandle mesh;
        OverlayEffectHandle effect;
        /// The effect's own constant block, of exactly the size it declared. Read during the
        /// call and not kept; ignored, and may be null, when the effect declared none.
        const void *parameters = nullptr;
        /// Bound where the pixel shader declares g_OverlayTexture. An effect whose shader
        /// declares none ignores it.
        TextureHandle texture;
    };
} // namespace BreadEngine
