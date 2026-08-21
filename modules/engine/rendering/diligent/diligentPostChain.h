#pragma once
#include <array>
#include <vector>

#include "diligentInternal.h"

namespace BreadEngine {
    /**
     * Everything the frame passes through once the scene and the background have been drawn:
     * fog over the whole image, a glow built from it, a blur by how far a pixel sits from the
     * focus distance, and the one composite that tone maps, grades and encodes the result into
     * the displayable target.
     *
     * The order is the reason these four sit together rather than apart. Fog runs first so the
     * sky is fogged by the same view ray the geometry is; bloom runs after it so what glows is
     * the frame as it will be seen; depth of field runs after bloom so a blurred highlight
     * spreads into the bokeh disk it would through a real lens rather than staying a sharp
     * point; the composite runs last because it is the only encode, and everything before it
     * works on one linear image.
     *
     * Owns its pipelines and the chain's own targets and nothing else. The scene's colour,
     * depth and output textures are handed in per pass instead of kept, because they are
     * reallocated whenever the target resizes and a copy here would outlive them.
     */
    class PostChain
    {
    public:
        /// Compiles the shaders and builds every pipeline the chain draws through. Each of the
        /// three can fail on its own and reports it; a failed one leaves its pass skipped
        /// rather than taking the frame down.
        void initialize(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context);

        void shutdown();

        /// Reads the tone mapping, colour grading, fog and bloom blocks. The rest of the
        /// settings belong to other parts of the renderer, which read them from the same call.
        void setSettings(const EnvironmentSettings &settings);

        /// Drops the bloom chain and the depth-of-field target, both sized to a scene target
        /// that is going away.
        void releaseTargets();

        /**
         * Blends fog over everything in the scene target, geometry and background alike.
         *
         * @param encoding the exponent the composite will leave the frame through, which is
         *        also what the authored fog colour has to be decoded by to reach linear.
         */
        void drawFog(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneDepth, const CameraView &camera,
                     const Matrix &viewProjection, float encoding);

        /// Filters the frame down the chain, blurs it back up, and blends the glow over the
        /// scene target in the linear space it belongs in.
        void drawBloom(Diligent::ITexture *sceneColor);

        /**
         * Blurs @p sceneColor by depth of field into the chain's own target and returns
         * whichever texture holds the result the composite should read - @p sceneColor itself,
         * untouched, while the effect is disabled or has nothing left to blur, or the blurred
         * copy once it ran. Never mutates @p sceneColor: the gather reads a wide neighbourhood
         * of texels a pass cannot also be writing.
         */
        [[nodiscard]] Diligent::ITexture *drawDepthOfField(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneDepth,
                                                            const CameraView &camera, const Matrix &viewProjection);

        /// Tone maps and grades @p sceneColor into @p sceneOutput, raising the graded result
        /// to @p encoding on the way. Leaves that target bound, which is what the overlay and
        /// the blit both go on to use.
        void composite(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneOutput, float encoding);

    private:
        /// Blend modes the glow can be combined through, which is every BloomMode but
        /// Disabled. A pipeline's blend state is fixed once it exists, so this is also how
        /// many combine pipelines there are.
        static constexpr size_t BLOOM_BLEND_MODE_COUNT = 3;

        /// Everything the composite pass turns the linear scene into a displayable image
        /// with. Held as values rather than as the parameter blocks themselves: the blocks
        /// belong to the project's settings and are handed over by reference per frame.
        struct CompositeState
        {
            TonemapMode tonemap = TonemapMode::Linear;
            float exposure = 1.0f;
            float whitePoint = 1.0f;
            float brightness = 1.0f;
            float contrast = 1.0f;
            float saturation = 1.0f;
        };

        /// What the fog pass turns a view ray's length into a fog amount with. Held as values
        /// for the same reason CompositeState is.
        struct FogState
        {
            FogMode mode = FogMode::Disabled;
            /// Authored, and decoded into the scene's linear space where it is uploaded - the
            /// same treatment the clear colour gets, and for the same reason: fog is looked at
            /// rather than lit with.
            Color color = WHITE;
            float start = 0.0f;
            float end = 0.0f;
            float density = 0.0f;
            float height = 0.0f;
            float heightFalloff = 0.0f;
            float skyAffect = 0.0f;
        };

        /// What the chain is built and combined with. Held as values for the same reason
        /// CompositeState and FogState are.
        struct BloomState
        {
            BloomMode mode = BloomMode::Disabled;
            /// How much of the chain the scene target has room for to actually build, from
            /// one level at the fine end to all of them.
            float levels = 0.0f;
            float intensity = 0.0f;
            float threshold = 0.0f;
            float softThreshold = 0.0f;
            /// Width of the upsample tent, in texels of the level it reads.
            float filterRadius = 1.0f;
        };

        void createCompositePipeline();

        void createFogPipeline();

        void createDepthOfFieldPipeline();

        /// Sizes the chain's own depth-of-field target to @p sceneColor. Nothing announces
        /// either changing, so it is compared rather than trusted.
        void resizeDepthOfFieldTarget(const Diligent::ITexture *sceneColor);

        /// Builds the two passes the chain is filled with, and the three the glow meets the
        /// scene through - one per blend mode.
        void createBloomPipelines();

        /// Sizes the chain to @p sceneColor and to the level count currently asked for.
        /// Nothing announces either changing, so both are compared rather than trusted.
        void resizeBloomChain(const Diligent::ITexture *sceneColor);

        /// Draws one step of the chain: @p source, read through @p binding, into @p target.
        /// The constants are the caller's, because they are what differs between filtering
        /// down the chain, blurring back up it, and reaching the scene.
        void drawBloomStep(Diligent::IPipelineState *pipeline, Diligent::IShaderResourceBinding *binding,
                           Diligent::ITexture *source, Diligent::ITextureView *target);

        /// Borrowed from the renderer, which outlives this and shuts it down before releasing
        /// either.
        Diligent::IRenderDevice *_device = nullptr;
        Diligent::IDeviceContext *_context = nullptr;

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _compositePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _compositeBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _postConstants;
        CompositeState _composite{};

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _fogPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _fogBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _fogConstants;
        FogState _fog{};

        /// What the depth-of-field pass turns a pixel's own distance from the focus plane into
        /// a blur radius with. Held as values for the same reason FogState is.
        struct DOFState
        {
            DepthOfFieldMode mode = DepthOfFieldMode::Disabled;
            float focusPoint = 10.0f;
            float focusScale = 1.0f;
            float nearScale = 1.0f;
            float maxBlurSize = 20.0f;
        };

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _dofPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _dofBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _dofConstants;
        /// The chain's own copy of the scene, blurred. Full scene size rather than the bloom
        /// chain's downsampled levels: a gather this wide is already the effect, not something
        /// a coarser mip could stand in for without losing exactly the near/far separation the
        /// blur radius is built from.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _dofTarget;
        DOFState _dof{};

        /// Filters the scene down the chain and blurs it back up. Two pipelines over one
        /// constant buffer, because the two directions differ only in their kernel and in
        /// whether they blend into what is already there.
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _bloomDownsamplePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _bloomDownsampleBinding;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _bloomUpsamplePipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _bloomUpsampleBinding;
        /// One per BloomMode past Disabled. The combine is the last upsample by another name -
        /// same shaders, same resources - and the modes differ only in how the result meets
        /// the scene, which is blend state and so fixed once a pipeline exists.
        std::array<Diligent::RefCntAutoPtr<Diligent::IPipelineState>, BLOOM_BLEND_MODE_COUNT> _bloomCombinePipelines;
        std::array<Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>, BLOOM_BLEND_MODE_COUNT> _bloomCombineBindings;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _bloomConstants;
        /// The chain itself, finest first, each level half the size of the one before it. A
        /// texture per level rather than the mips of one, because a level is a render target
        /// and a shader resource in the same pass and only whole textures are both everywhere.
        std::vector<Diligent::RefCntAutoPtr<Diligent::ITexture>> _bloomChain;
        BloomState _bloom{};
    };
} // namespace BreadEngine
