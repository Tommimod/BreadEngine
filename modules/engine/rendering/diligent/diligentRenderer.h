#pragma once
#include <array>
#include <future>
#include <vector>

#include "diligentEnvironmentMaps.h"
#include "diligentInternal.h"
#include "diligentPostChain.h"
#include "diligentShadowPass.h"

#include "../resourcePool.h"

namespace BreadEngine {
    /**
     * Renders through DiligentEngine, attached to the OpenGL context raylib already created
     * so the scene target is a GL texture rlgl can composite and draw over without a copy.
     *
     * Owns the device, the scene's own targets, the pools every handle resolves through and
     * the scene pass that draws into them. The passes on either side of that one are members
     * with their own state: ShadowPass fills the maps the scene shades with, EnvironmentMaps
     * holds the sky it is drawn against and lit by, and PostChain finishes the frame.
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

        [[nodiscard]] CubemapHandle loadCubemap(const std::string &path,
                                                const SkyboxCubemapParameters &settings) override;

        [[nodiscard]] bool isCubemapReady(CubemapHandle handle) const override;

        [[nodiscard]] CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky) override;

        void destroyCubemap(CubemapHandle handle) override;

        [[nodiscard]] AmbientMapHandle createAmbientMap(CubemapHandle cubemap) override;

        void destroyAmbientMap(AmbientMapHandle handle) override;

    private:
        /// Material texture slots, in the order MaterialDesc declares them.
        static constexpr size_t MATERIAL_TEXTURE_COUNT = 4;

        /// Encoding exponents for the two OutputColorSpace values: the sRGB approximation, and
        /// the identity that leaves the linear result alone.
        static constexpr float GAMMA_ENCODE_EXPONENT = 1.0f / 2.2f;
        static constexpr float LINEAR_ENCODE_EXPONENT = 1.0f;

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
        /// The exponent the frame leaves the composite through, and so the one every authored
        /// colour written into the linear scene target has to be decoded by. Set once from the
        /// project's output colour space; read by the clear, by the sky bakes and by the
        /// composite, which is why it belongs to the renderer rather than to any one of them.
        float _outputEncoding = GAMMA_ENCODE_EXPONENT;
        /// False while the scene target follows the window rather than a caller-chosen size.
        bool _hasExplicitTarget = false;

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _scenePipeline;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _frameConstants;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _drawConstants;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _lightConstants;
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
        /// Every shadow map the scene pass samples, filled ahead of it each frame.
        ShadowPass _shadowPass;
        /// The sky the frame is drawn against and the maps it is lit by, with the pools every
        /// cube map and ambient map handle resolves through.
        EnvironmentMaps _environment;
        /// Fog, bloom and the composite, in the order the frame passes through them.
        PostChain _postChain;

        void createSceneTarget(int width, int height);

        void releaseSceneTarget();

        /// Compiles the shaders and builds the one pipeline the scene pass draws through.
        void createScenePipeline();

        /// Points @p slot's environment variables at @p map, or at the fallback cube when it
        /// names none.
        void bindAmbientMap(MaterialSlot &slot, AmbientMapHandle map);

        /// Builds the 1x1 stand-ins bound wherever a material leaves a texture slot unset.
        void createMaterialFallbacks();

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

        /// Undoes the bindings and the GL state Diligent changed behind rlgl's back, so
        /// raylib's next draw lands where and how it expects.
        void yieldToRaylib();
    };
} // namespace BreadEngine
