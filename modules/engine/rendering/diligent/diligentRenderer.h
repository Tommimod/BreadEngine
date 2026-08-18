#pragma once
#include <array>
#include <future>
#include <span>
#include <vector>

#include <Buffer.h>
#include <DeviceContext.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <ShaderResourceBinding.h>
#include <TextureLoader.h>
// raylib's raymath.h defines PI as a macro, and Diligent declares a constant of that name. A
// translation unit that reaches raylib first would otherwise break on this include alone.
#pragma push_macro("PI")
#undef PI
#include <Components/interface/ShadowMapManager.hpp>
#pragma pop_macro("PI")

#include "../IRenderer.h"
#include "../resourcePool.h"

// float16 is raymath's, and it is the form every matrix reaches the GPU in. After IRenderer.h,
// because raymath declares raylib's vector types unguarded and raylib.h has to win.
#include "raymath.h"

namespace BreadEngine {
    /**
     * Renders through DiligentEngine, attached to the OpenGL context raylib already created
     * so the scene target is a GL texture rlgl can composite and draw over without a copy.
     */
    class DiligentRenderer final : public IRenderer
    {
    public:
        [[nodiscard]] const char *getBackendName() const override { return "Diligent"; }

        void initialize(int sceneWidth, int sceneHeight) override;

        void shutdown() override;

        void resizeSceneTarget(int width, int height) override;

        void setOutputColorSpace(OutputColorSpace colorSpace) override;

        void beginScene(const CameraView &camera) override;

        void endScene() override;

        void beginSceneOverlay() override;

        void endSceneOverlay() override;

        void drawSceneTexture(Rectangle destination) override;

        [[nodiscard]] LightHandle createLight(LightType type) override;

        void destroyLight(LightHandle handle) override;

        [[nodiscard]] bool isLightValid(LightHandle handle) const override;

        void updateLight(LightHandle handle, const LightState &state) override;

        [[nodiscard]] TextureHandle createTexture(const TextureDesc &desc) override;

        void destroyTexture(TextureHandle handle) override;

        [[nodiscard]] TextureSize getTextureSize(TextureHandle handle) override;

        [[nodiscard]] MaterialHandle createMaterial(const MaterialDesc &desc) override;

        void destroyMaterial(MaterialHandle handle) override;

        [[nodiscard]] MeshHandle createMesh(const MeshData &data) override;

        void destroyMesh(MeshHandle handle) override;

        void drawMesh(const MeshDrawDesc &draw) override;

        void setEnvironment(const EnvironmentSettings &settings) override;

        [[nodiscard]] CubemapHandle loadCubemap(const std::string &path) override;

        [[nodiscard]] CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky) override;

        void destroyCubemap(CubemapHandle handle) override;

        [[nodiscard]] AmbientMapHandle createAmbientMap(CubemapHandle cubemap) override;

        void destroyAmbientMap(AmbientMapHandle handle) override;

    private:
        /// Material texture slots, in the order MaterialDesc declares them.
        static constexpr size_t MATERIAL_TEXTURE_COUNT = 4;

        /// Slices of the spot shadow array, and so how many spot lights can cast at once.
        static constexpr size_t MAX_SPOT_SHADOWS = 4;

        /// Cubes of the omni shadow array, and so how many omni lights can cast at once. Each
        /// one costs six depth passes rather than the spot's one, which is what keeps the
        /// count and the face resolution smaller than the spot array's.
        static constexpr size_t MAX_OMNI_SHADOWS = 4;

        /// Faces of a cube map, in the order every graphics API agrees on: +X, -X, +Y, -Y,
        /// +Z, -Z. Diligent indexes a cube array in layer-faces, so a cube's first face is at
        /// slice * CUBE_FACE_COUNT.
        static constexpr size_t CUBE_FACE_COUNT = 6;

        /// Encoding exponents for the two OutputColorSpace values: the sRGB approximation, and
        /// the identity that leaves the linear result alone.
        static constexpr float GAMMA_ENCODE_EXPONENT = 1.0f / 2.2f;
        static constexpr float LINEAR_ENCODE_EXPONENT = 1.0f;

        struct MeshSlot
        {
            Diligent::RefCntAutoPtr<Diligent::IBuffer> vertices;
            Diligent::RefCntAutoPtr<Diligent::IBuffer> indices;
            Diligent::Uint32 indexCount = 0;
        };

        struct TextureSlot
        {
            TextureDesc desc;
            /// Decoding produces a loader, which then builds the texture on the thread that
            /// owns the device. The loader is dropped once it has.
            Diligent::RefCntAutoPtr<Diligent::ITextureLoader> loader;
            Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
            /// The job writes into this slot, so every path that frees or recycles the slot
            /// has to wait on it first - dropping the future does not wait on its own.
            std::future<void> decodeJob;
            bool uploaded = false;
        };

        /// A draw the scene pass has taken but not yet issued: the pass runs between
        /// beginScene and endScene, and the render target is not bound until endScene.
        struct DrawItem
        {
            MeshHandle mesh;
            MaterialHandle material;
            Matrix model;
            bool castShadows = true;
        };

        /// Everything the composite pass turns the linear scene into a displayable image
        /// with. Held as values rather than as the parameter blocks themselves: the blocks
        /// belong to the project's settings and are handed over by reference per frame.
        struct PostState
        {
            TonemapMode tonemap = TonemapMode::Linear;
            float exposure = 1.0f;
            float whitePoint = 1.0f;
            float brightness = 1.0f;
            float contrast = 1.0f;
            float saturation = 1.0f;
            /// Exponent the graded result is raised to on the way to the target.
            float encoding = GAMMA_ENCODE_EXPONENT;
        };

        /// A material is exactly its binding, and mutable variables cannot be re-pointed, so
        /// the texture set is fixed for as long as the material exists. The environment cubes
        /// are the exception and are dynamic: they are replaced whenever the sky is rebaked,
        /// which neither a static nor a mutable variable could survive.
        struct MaterialSlot
        {
            Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> binding;
            /// Resolved once, so keeping a binding current costs a comparison per draw rather
            /// than two lookups by name.
            Diligent::IShaderResourceVariable *irradiance = nullptr;
            Diligent::IShaderResourceVariable *prefiltered = nullptr;
            /// Which ambient map those two variables currently point at. The inspector never
            /// announces that the environment changed, so this is compared rather than trusted.
            AmbientMapHandle ambientMap{};
        };

        /// A light that reached the shader this frame, and what was rendered for it. The two
        /// shadow kinds are separate because they are separate mechanisms: a cascade array
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

        /// What the scene pass reads shadowing from.
        struct ShadowConstants
        {
            Diligent::ShadowMapAttribs cascades;
            Diligent::float4x4 spotTransforms[MAX_SPOT_SHADOWS];
            /// x is the width of that slice's filter kernel, in shadow map UV.
            Diligent::float4 spotParams[MAX_SPOT_SHADOWS];
            /// An omni light gets no transform: the depth its cube holds depends only on the
            /// largest component of the direction to the surface, so the scene pass rebuilds it
            /// from the light's own position without knowing which face it will land on.
            /// xyz is that position, w the far/(far - near) of the cube's projection.
            Diligent::float4 omniPosition[MAX_OMNI_SHADOWS];
            /// x is near * far / (far - near), the other half of that depth; y and z turn a
            /// distance from the light into the offset one filter step covers, and into the
            /// distance a lookup steps off the surface before it compares.
            Diligent::float4 omniParams[MAX_OMNI_SHADOWS];
        };

        /// What a cube map bake pass reads. The three face axes lead so that the
        /// equirectangular shader can declare just those and share the buffer: a constant
        /// block may be a prefix of the buffer behind it, but not a rearrangement of one.
        struct SkyBakeConstants
        {
            Diligent::float4 faceRight;
            Diligent::float4 faceUp;
            Diligent::float4 faceForward;
            /// Hosek-Wilkie's nine coefficients, three channels per element.
            Diligent::float4 coefficients[9];
            Diligent::float4 radiance;
            /// xyz is the direction to the sun, w the cosine of its disc's angular radius.
            Diligent::float4 sun;
            Diligent::float4 sunColor;
            /// rgb an artistic multiplier over the model, a the overall energy.
            Diligent::float4 tint;
            Diligent::float4 ground;
        };

        /// What the background pass reads.
        struct SkyboxConstants
        {
            float16 inverseViewProjection;
            Diligent::float4 cameraPosition;
            /// Rotation applied to the view direction, as a quaternion.
            Diligent::float4 rotation;
            /// x the energy multiplier, y the mip level the blur setting selects.
            Diligent::float4 params;
        };

        /// What the two image-based lighting bake passes read. The face axes lead, as the sky's
        /// do, because a cube bake is the same pass whatever it is filling.
        struct IblBakeConstants
        {
            Diligent::float4 faceRight;
            Diligent::float4 faceUp;
            Diligent::float4 faceForward;
            /// x is the perceptual roughness the mip being filled stands for, which only the
            /// reflection pass reads; y the source cube's face size in texels and z its mip
            /// count, which together decide how coarse a mip each sample is read from; w how
            /// many directions to sample.
            Diligent::float4 filter;
        };

        struct CubemapSlot
        {
            Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
        };

        /// What one environment precomputes to: the irradiance arriving from every direction
        /// at once, and the reflection of that environment at each roughness, one per mip.
        struct AmbientMapSlot
        {
            Diligent::RefCntAutoPtr<Diligent::ITexture> irradiance;
            Diligent::RefCntAutoPtr<Diligent::ITexture> prefiltered;
        };

        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> _device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> _context;
        /// What the scene pass shades into: linear, floating point, and unbounded, so a
        /// value brighter than white survives to be tone mapped rather than clipping on write.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneColor;
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneDepth;
        /// What the composite pass writes and everything downstream reads: the displayable,
        /// already-encoded image. The overlay draws here, and this is what gets blitted.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneOutput;
        /// A raylib framebuffer over the output colour and the scene's own depth, so an rlgl
        /// overlay pass is occluded by scene geometry without being tone mapped with it.
        RenderTexture2D _overlay{};
        Color _clearColor = BLACK;
        /// False while the scene target follows the window rather than a caller-chosen size.
        bool _hasExplicitTarget = false;

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _scenePipeline;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _frameConstants;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _drawConstants;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _lightConstants;
        /// Bakes Hosek-Wilkie into a cube face, and unwraps an equirectangular image into
        /// one. Two pipelines over one constant buffer, because the passes differ only in
        /// where the radiance for a direction comes from.
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _skyBakePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _skyBakeBinding;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _equirectBakePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _equirectBakeBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _skyBakeConstants;
        /// Convolves an environment cube into the two maps the scene pass shades ambient with.
        /// Separate pipelines over one constant buffer: the passes differ only in which
        /// distribution they sample the source with.
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _irradiancePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _irradianceBinding;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _prefilterPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _prefilterBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _iblBakeConstants;
        /// The environment-independent half of the split sum. It depends on nothing but the
        /// BRDF, so it is integrated once at startup and never again.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _brdfLut;
        /// Bound wherever a scene has no environment map. Nothing ever reads it - the shader
        /// takes the flat ambient colour on that branch - but a dynamic variable still has to
        /// point at something for a draw to validate.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _ambientFallback;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _skyboxPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _skyboxBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _skyboxConstants;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _compositePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _compositeBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _postConstants;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _shadowPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _shadowBinding;
        /// The cascade the shadow pass is currently filling; one matrix, rewritten per cascade.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _shadowPassConstants;
        /// What the scene pass reads back: the cascade transforms and the filtering parameters.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _shadowConstants;
        Diligent::ShadowMapManager _shadowMap;
        /// The spot lights' shadow maps, one array slice each. Not the cascade manager's job:
        /// a spot needs a single perspective map, not a set fitted to the camera's frustum.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> _spotShadowSRV;
        std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, MAX_SPOT_SHADOWS> _spotShadowDSVs;
        /// The omni lights' shadow maps, one cube each. A cube rather than six flat slices so
        /// the scene pass picks the face from the direction it is already holding.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> _omniShadowSRV;
        std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, MAX_OMNI_SHADOWS * CUBE_FACE_COUNT> _omniShadowDSVs;
        ShadowConstants _shadowData;
        /// What stands in wherever a material leaves a texture slot unset.
        std::array<Diligent::RefCntAutoPtr<Diligent::ITexture>, MATERIAL_TEXTURE_COUNT> _materialFallbacks;
        ResourcePool<MaterialSlot, MaterialHandle> _materials;
        ResourcePool<MeshSlot, MeshHandle> _meshes;
        ResourcePool<TextureSlot, TextureHandle> _textures;
        ResourcePool<LightState, LightHandle> _lights;
        ResourcePool<CubemapSlot, CubemapHandle> _cubemaps;
        ResourcePool<AmbientMapSlot, AmbientMapHandle> _ambientMaps;
        /// The active lights of the frame being submitted, rebuilt per frame. A member only
        /// so the per-frame gather reuses one allocation.
        std::vector<VisibleLight> _visibleLights;
        std::vector<DrawItem> _draws;
        Matrix _viewProjection{};
        /// Kept whole rather than reduced to a matrix: fitting the shadow cascades needs the
        /// camera's basis and its field of view, not just the transform they combine into.
        CameraView _camera{};
        Color _ambientColor = BLACK;
        float _ambientEnergy = 0.0f;
        /// The environment the scene pass shades ambient from. Invalid falls back to the flat
        /// colour above, which is what a scene with no skybox wants.
        AmbientMapHandle _ambientMap{};
        /// The environment cube the background pass draws, and how. Invalid leaves the frame
        /// on the flat clear colour, which is what a scene with no skybox wants.
        CubemapHandle _sky{};
        Quaternion _skyRotation{0.0f, 0.0f, 0.0f, 1.0f};
        float _skyEnergy = 1.0f;
        float _skyBlur = 0.0f;
        PostState _post{};

        void createSceneTarget(int width, int height);

        void releaseSceneTarget();

        /// Compiles the shaders and builds the one pipeline the scene pass draws through.
        void createScenePipeline();

        /// Builds the fullscreen pass that resolves the linear scene into the output texture.
        void createCompositePipeline();

        /// Resolves an #include from the engine's own shader directory or, failing that, from
        /// DiligentFX - whose .fxh files are compiled into the library rather than shipped.
        [[nodiscard]] Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> createShaderSources() const;

        /// Builds the two cube map bake passes and the background pass that samples the result.
        void createSkyPipelines();

        /**
         * Allocates a cube map of @p size and fills its six faces with @p pipeline, rewriting
         * @p constants' face axes for each. Generates the mip chain, which is what the blur
         * setting samples down.
         */
        [[nodiscard]] CubemapHandle bakeCubemap(const char *name, int size, Diligent::IPipelineState *pipeline,
                                                Diligent::IShaderResourceBinding *binding, SkyBakeConstants &constants);

        /// Builds the two precompute passes and integrates the BRDF table they are sampled
        /// alongside. Runs before the scene pipeline, which binds that table for its lifetime.
        void createIblPipelines();

        /// Integrates the preintegrated GGX table. Depends on nothing but the shading model,
        /// so it runs once and the pass that fills it is discarded with it.
        void precomputeBrdfLut();

        /**
         * Allocates a cube of @p size with @p mipCount levels and fills every face of every one
         * of them with @p pipeline, rewriting @p constants' face axes and the roughness the mip
         * stands for before each draw.
         */
        [[nodiscard]] Diligent::RefCntAutoPtr<Diligent::ITexture> bakeIblCube(
            const char *name, int size, Diligent::Uint32 mipCount, Diligent::IPipelineState *pipeline,
            Diligent::IShaderResourceBinding *binding, IblBakeConstants &constants);

        /// Points @p slot's environment variables at @p map, or at the fallback cube when it
        /// names none.
        void bindAmbientMap(MaterialSlot &slot, AmbientMapHandle map);

        /// Draws the environment cube behind everything the scene pass rendered.
        void drawSkybox();

        /// Tone maps and grades the scene into _sceneOutput. Leaves that target bound, which
        /// is what the overlay and the blit both go on to use.
        void composite();

        /// Allocates all three shadow arrays and the comparison sampler the scene pass reads
        /// them with.
        void createShadowMaps();

        /// Allocates one depth array of @p sliceDSVs.size() slices, the view the scene pass
        /// samples it through, and one depth-stencil view per slice for the passes that fill it.
        void createShadowArray(const char *name, Diligent::RESOURCE_DIMENSION dimension, Diligent::Uint32 resolution,
                               Diligent::ISampler *comparisonSampler,
                               Diligent::RefCntAutoPtr<Diligent::ITextureView> &srv,
                               std::span<Diligent::RefCntAutoPtr<Diligent::ITextureView>> sliceDSVs);

        /// Draws every shadow-casting item of the frame into @p target, seen through
        /// @p worldToLightClip. Already in upload order, because its two callers arrive at it
        /// from different places - one from raylib's math, one out of DiligentFX.
        void renderShadowCasters(Diligent::ITextureView *target, const float16 &worldToLightClip);

        /// Fits and fills the cascade array for one directional light.
        void renderCascades(const LightState &light);

        /// Fills one slice of the spot shadow array, and records the transform to sample it with.
        void renderSpotShadow(const LightState &light, int slice);

        /// Fills all six faces of one cube of the omni shadow array, and records what the scene
        /// pass rebuilds their depth with.
        void renderOmniShadow(const LightState &light, int slice);

        /// Builds the depth-only pipeline the cascades are filled through.
        void createShadowPipeline();

        /// Fills every shadow map the frame's assignments call for. Leaves nothing bound: the
        /// scene pass rebinds its own target.
        void renderShadowMaps();

        /// Builds the 1x1 stand-ins bound wherever a material leaves a texture slot unset.
        void createMaterialFallbacks();

        /// Puts back the pixel-unpack state raylib's own texture uploads depend on. Call
        /// after anything that hands pixels to Diligent.
        static void restoreRaylibPixelStore();

        /// Overwrites a whole dynamic constant buffer. Matrices go in as MatrixToFloatV
        /// leaves them - the column-major order rlgl uploads its own in, which is what the
        /// shaders' cbuffer packing expects.
        void uploadConstants(Diligent::IBuffer *buffer, const void *data, size_t size);

        /// Joins the background decode and creates the GPU texture, unless already created.
        void finalizeTexture(TextureSlot &slot);

        /// What a material's texture slot binds: the texture behind @p handle once it is
        /// resident, or @p fallback when the material leaves the slot unset.
        [[nodiscard]] Diligent::ITextureView *materialTextureView(TextureHandle handle, Diligent::ITexture *fallback);

        static Diligent::SamplerDesc toNative(TextureFilterMode filter, TextureWrapMode wrap);

        static Diligent::TEXTURE_ADDRESS_MODE toNative(TextureWrapMode wrap);

        /// Issues everything the scene pass collected, into the currently bound target.
        void submitDraws();

        /// Uploads _visibleLights, shadow assignments and all, into the light constant buffer.
        void uploadLights();

        /// Fills _visibleLights with the lights the shader should see, most significant first.
        /// Ordering only costs anything when there are more lights than the buffer has room for.
        void selectVisibleLights(size_t capacity);

        /// Hands out the frame's shadow maps: the cascades to the first directional caster, and
        /// a slice of the spot array to each of the next few spot casters.
        void assignShadowSlots();

        /// Undoes the bindings and the GL state Diligent changed behind rlgl's back, so
        /// raylib's next draw lands where and how it expects.
        void yieldToRaylib();
    };
} // namespace BreadEngine
