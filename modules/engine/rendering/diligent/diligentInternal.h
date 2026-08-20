#pragma once
#include "diligentRenderer.h"

/**
 * The half of the renderer's internals that more than one of its translation units needs:
 * the scene target's formats, the layout of the constant buffers the scene shaders read,
 * the cube-face axis table, and the one conversion into the scene's linear space.
 *
 * Everything a single concern owns lives next to that concern's code instead - the shadow
 * resolutions in diligentLighting.cpp, the sky and precompute sizes in
 * diligentEnvironment.cpp, the post chain's own blocks in diligentPost.cpp. A constant that
 * gains a second reader belongs here; one that loses its second reader belongs back there.
 */
namespace BreadEngine {
    /// Where the engine's shader sources sit relative to the executable. Read by the one
    /// helper that resolves them and by the shadow pass, which builds its own factory.
    constexpr const char *SHADER_DIRECTORY = "shaders";

    /// A spot covers one cone rather than the whole visible world, so it needs far less of a
    /// map than a cascade does. The scene pass is told this so it can size its filter kernel
    /// in texels, which is why it is not the shadow pass's own business alone.
    constexpr Diligent::Uint32 SPOT_SHADOW_RESOLUTION = 1024;

    /// How many mips of the reflection cube are filled, and so how many roughness steps the
    /// scene pass interpolates between. Mip 0 is a mirror and the last is fully rough; below
    /// four levels the steps become visible as bands on a curved surface. Shared because the
    /// precompute fills them and the scene pass picks between them.
    constexpr Diligent::Uint32 PREFILTERED_CUBE_MIPS = 6;

    /// The scene target's formats are fixed, so every pipeline that renders into it can be
    /// built against them once instead of being rebuilt when the target is resized. The scene
    /// shades into a float target because a light brighter than white has to survive as far as
    /// the tone mapper; only the composite pass's output is bounded to what a screen can show.
    constexpr Diligent::TEXTURE_FORMAT SCENE_COLOR_FORMAT = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    constexpr Diligent::TEXTURE_FORMAT SCENE_DEPTH_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;
    constexpr Diligent::TEXTURE_FORMAT SCENE_OUTPUT_FORMAT = Diligent::TEX_FORMAT_RGBA8_UNORM;

    /**
     * Mirrors scene.vsh's cbuffers. Everything is a float4 or a float4x4 on purpose: those
     * are the only members whose std140 placement is the same as their placement here, so the
     * struct and the shader cannot drift apart over padding.
     */
    struct SceneFrameConstants
    {
        float16 viewProjection;
        Vector4 cameraPosition;
        /// xyz is the direction the camera looks in. The pixel shader projects onto it to get
        /// the camera-space depth the cascade selection compares against.
        Vector4 cameraForward;
        /// rgb is the ambient colour used where no environment map is bound, w the energy.
        Vector4 ambientColor;
        /// Turns a world direction into the environment cube's space, as a quaternion. Already
        /// inverted: the authored rotation turns the sky, and this turns the lookup.
        Vector4 skyRotation;
        /// x is 1 while an environment map is bound, y the highest mip of the reflection cube.
        Vector4 ambientParams;
    };

    struct SceneDrawConstants
    {
        float16 model;
        float16 normalMatrix;
    };

    /// How many lights the pixel shader loops over. The shader is told this number rather than
    /// repeating it, so the array and the loop cannot disagree.
    constexpr size_t MAX_SCENE_LIGHTS = 32;

    /// One light as the shader reads it, with everything the pixel shader would otherwise have
    /// to derive per pixel folded in on the CPU.
    struct SceneLight
    {
        /// xyz is the world position, meaningless for a directional light; w is the LightType.
        Vector4 positionType;
        /// xyz is the direction the light travels, so a surface points back along it.
        Vector4 direction;
        /// rgb is the colour, w the intensity.
        Vector4 color;
        /// x is 1/range², y the cosine of the spot's half-angle, z the reciprocal of the
        /// cosine span its falloff covers, w whether the cascades were fitted to this light.
        Vector4 attenuation;
        /// x is the slice of the spot shadow array rendered for this light, y the cube of the
        /// omni one; either is -1 when this light has no map of that kind.
        Vector4 shadow;
    };

    struct SceneLightConstants
    {
        /// x is how many entries of the array are live, y and z how far along the spot and omni
        /// shadow arrays this frame filled.
        Vector4 count;
        SceneLight lights[MAX_SCENE_LIGHTS];
    };

    /// Faces of a cube map, in the order every graphics API agrees on, as the axes one face's
    /// texels span. They are the direction-to-texel rule read backwards rather than anything
    /// intuitive: for +X that rule is s = -z, t = -y, so u runs along -Z and v runs *down*
    /// along -Y. Every one of the six has v pointing the way that feels upside down, which is
    /// precisely why a wrong one mirrors a face without failing anywhere - the omni shadow cube
    /// pays for the same table.
    constexpr Vector3 CUBE_FACE_RIGHT[]{
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
    };
    constexpr Vector3 CUBE_FACE_UP[]{
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
    };
    constexpr Vector3 CUBE_FACE_FORWARD[]{
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}
    };

    /// An authored colour as the linear scene target needs it. A colour is picked in the
    /// encoded space a screen shows, and the composite pass encodes on the way back out, so
    /// what is written here has to be decoded by exactly the inverse of that encode - which
    /// also makes the whole round trip an identity when the output is asked to stay linear.
    inline Vector4 toSceneLinear(const Color color, const float encoding)
    {
        const auto normalized = ColorNormalize(color);
        const float exponent = 1.0f / encoding;
        return {
            std::pow(normalized.x, exponent), std::pow(normalized.y, exponent),
            std::pow(normalized.z, exponent), normalized.w
        };
    }
} // namespace BreadEngine
