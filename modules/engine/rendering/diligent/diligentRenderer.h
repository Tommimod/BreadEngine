#pragma once
#include <array>
#include <future>
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

        /// A material is exactly its binding, and mutable variables cannot be re-pointed, so
        /// the texture set is fixed for as long as the material exists.
        using MaterialSlot = Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>;

        /// A light that reached the shader this frame, and what was rendered for it. The two
        /// shadow kinds are separate because they are separate mechanisms: a cascade array
        /// fitted to the camera, or one slice of a fixed perspective map.
        struct VisibleLight
        {
            const LightState *light = nullptr;
            /// Slice of the spot shadow array this light was rendered into, or -1.
            int spotShadowSlice = -1;
            bool ownsCascades = false;
        };

        /// What the scene pass reads shadowing from.
        struct ShadowConstants
        {
            Diligent::ShadowMapAttribs cascades;
            Diligent::float4x4 spotTransforms[MAX_SPOT_SHADOWS];
            /// x is the width of that slice's filter kernel, in shadow map UV.
            Diligent::float4 spotParams[MAX_SPOT_SHADOWS];
        };

        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> _device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> _context;
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneColor;
        Diligent::RefCntAutoPtr<Diligent::ITexture> _sceneDepth;
        /// A raylib framebuffer over the same two GL textures, so an rlgl overlay pass lands
        /// in the attachments Diligent just rendered into instead of a copy of the colour.
        RenderTexture2D _overlay{};
        Color _clearColor = BLACK;
        /// False while the scene target follows the window rather than a caller-chosen size.
        bool _hasExplicitTarget = false;

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _scenePipeline;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _frameConstants;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _drawConstants;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _lightConstants;
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
        ShadowConstants _shadowData;
        /// What stands in wherever a material leaves a texture slot unset.
        std::array<Diligent::RefCntAutoPtr<Diligent::ITexture>, MATERIAL_TEXTURE_COUNT> _materialFallbacks;
        ResourcePool<MaterialSlot, MaterialHandle> _materials;
        ResourcePool<MeshSlot, MeshHandle> _meshes;
        ResourcePool<TextureSlot, TextureHandle> _textures;
        ResourcePool<LightState, LightHandle> _lights;
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
        /// Exponent the shader raises its linear result to on the way to the target.
        float _outputEncoding = GAMMA_ENCODE_EXPONENT;

        void createSceneTarget(int width, int height);

        void releaseSceneTarget();

        /// Compiles the shaders and builds the one pipeline the scene pass draws through.
        void createScenePipeline();

        /// Allocates both shadow arrays and the comparison sampler the scene pass reads them with.
        void createShadowMaps();

        /// Draws every shadow-casting item of the frame into @p target, seen through
        /// @p worldToLightClip. Already in upload order, because its two callers arrive at it
        /// from different places - one from raylib's math, one out of DiligentFX.
        void renderShadowCasters(Diligent::ITextureView *target, const float16 &worldToLightClip);

        /// Fits and fills the cascade array for one directional light.
        void renderCascades(const LightState &light);

        /// Fills one slice of the spot shadow array, and records the transform to sample it with.
        void renderSpotShadow(const LightState &light, int slice);

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
