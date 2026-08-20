#pragma once
#include <cmath>
#include <cstddef>

// raylib's raymath.h defines PI as a macro and Diligent declares a constant of that name, so
// a translation unit that reached raylib first would break on these includes alone. This is the
// only place the backend reaches Diligent, so the guard is here rather than at each of them.
#pragma push_macro("PI")
#undef PI
#include <BasicMath.hpp>
#include <Buffer.h>
#include <DeviceContext.h>
#include <EngineFactory.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <ShaderResourceBinding.h>
#include <Texture.h>
#include <TextureLoader.h>
#pragma pop_macro("PI")

#include "../IRenderer.h"

// float16 is raymath's, and it is the form every matrix reaches the GPU in. After IRenderer.h,
// because raymath declares raylib's vector types unguarded and raylib.h has to win.
#include "raymath.h"

/**
 * What the rest of the Diligent backend is built on: the one place its headers are reached
 * through, the scene target's formats, the layout of the constant buffers the scene shaders
 * read, the cube-face axis table, and the few helpers no single subsystem owns.
 *
 * Everything one concern owns lives with that concern instead - the shadow maps' own state in
 * diligentShadowPass.h, the sky and precompute sizes in diligentEnvironmentMaps.h, the post
 * chain's own blocks in diligentPostChain.h. A constant that gains a second reader belongs
 * here; one that loses its second reader belongs back there.
 */
namespace BreadEngine {
    /// Where the engine's shader sources sit relative to the executable. Read by the one
    /// helper that resolves them and by the shadow pass, which builds its own factory.
    constexpr const char *SHADER_DIRECTORY = "shaders";

    /// Slices of the spot shadow array and cubes of the omni one, and so how many lights of
    /// each kind may cast at once. An omni caster costs six depth passes where a spot costs
    /// one, which is what keeps its count - and its face resolution - from growing. The scene
    /// pass sizes its own arrays from these two, which is why they are not the shadow pass's
    /// own business alone.
    constexpr size_t MAX_SPOT_SHADOWS = 4;
    constexpr size_t MAX_OMNI_SHADOWS = 4;

    /// A spot covers one cone rather than the whole visible world, so it needs far less of a
    /// map than a cascade does. The scene pass is told this so it can size its filter kernel
    /// in texels, which is why it is not the shadow pass's own business alone.
    constexpr Diligent::Uint32 SPOT_SHADOW_RESOLUTION = 1024;

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

    /// One mesh as the GPU holds it. Both passes that draw geometry reach it the same way: the
    /// scene pass for the image, the shadow pass for the depth the image is shadowed by.
    struct MeshSlot
    {
        Diligent::RefCntAutoPtr<Diligent::IBuffer> vertices;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> indices;
        Diligent::Uint32 indexCount = 0;
    };

    /// A draw taken but not yet issued: draws arrive between beginScene and endScene, and no
    /// render target is bound until endScene. The shadow pass walks the same list first,
    /// skipping everything that does not cast.
    struct DrawItem
    {
        MeshHandle mesh;
        MaterialHandle material;
        Matrix model;
        bool castShadows = true;
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

    /// A light that reached the shader this frame, and what the shadow pass rendered for it.
    /// The two shadow kinds are separate because they are separate mechanisms: a cascade array
    /// fitted to the camera, or one slice of a fixed perspective map.
    struct VisibleLight
    {
        const LightState *light = nullptr;
        /// Slice of the spot shadow array this light was rendered into, or -1.
        int spotShadowSlice = -1;
        /// Cube of the omni shadow array this light was rendered into, or -1.
        int omniShadowSlice = -1;
        bool ownsCascades = false;
    };

    /// Faces of a cube map, in the order every graphics API agrees on: +X, -X, +Y, -Y, +Z, -Z.
    /// Diligent indexes a cube array in layer-faces, so a cube's first face is at
    /// slice * CUBE_FACE_COUNT.
    constexpr size_t CUBE_FACE_COUNT = 6;

    /// Those same six faces as the axes one face's texels span. They are the direction-to-texel
    /// rule read backwards rather than anything intuitive: for +X that rule is s = -z, t = -y,
    /// so u runs along -Z and v runs *down* along -Y. Every one of the six has v pointing the
    /// way that feels upside down, which is precisely why a wrong one mirrors a face without
    /// failing anywhere - the omni shadow cube pays for the same table.
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

    /// Resolves an #include from the engine's own shader directory or, failing that, from
    /// DiligentFX - whose .fxh files are compiled into the library rather than shipped.
    [[nodiscard]] Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> createShaderSources();

    /// Overwrites a whole dynamic constant buffer. Matrices go in as MatrixToFloatV leaves
    /// them - the column-major order rlgl uploads its own in, which is what the shaders'
    /// cbuffer packing expects.
    void uploadConstants(Diligent::IDeviceContext *context, Diligent::IBuffer *buffer, const void *data, size_t size);

    /// Puts back the pixel-unpack state raylib's own texture uploads depend on. Call after
    /// anything that hands pixels to Diligent.
    void restoreRaylibPixelStore();
} // namespace BreadEngine
