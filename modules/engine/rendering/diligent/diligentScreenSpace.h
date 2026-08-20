#pragma once
#include "diligentInternal.h"

namespace BreadEngine {
    /**
     * The two effects that shade the frame a second time from what the scene pass left on
     * screen: ambient occlusion, which takes back the ambient light a crevice never received,
     * and reflections, which replace the environment reflected in a surface with the part of
     * the scene actually in front of it.
     *
     * Both belong here rather than in the post chain because neither finishes the image -
     * they correct the lighting in it, and they correct the same one: occlusion darkens the
     * ambient the scene pass applied, reflections swap out the specular half of that same
     * ambient. So both run before the fog, on a frame still made of linear radiance, and both
     * read the surface target the scene pass wrote beside its colour.
     *
     * The order between them is not free either. Occlusion runs first and darkens the ambient
     * in place; reflections then have to remove what is actually in the target, which is the
     * darkened value - so the reflection pass reads the occlusion this one produced. With the
     * effect switched off it reads a white texel instead, which is the same arithmetic.
     *
     * Owns its pipelines and the occlusion targets. The scene's own textures are handed in per
     * pass rather than kept, because they are reallocated whenever the target resizes.
     */
    class ScreenSpaceEffects
    {
    public:
        /// Compiles the shaders and builds the pipelines both effects draw through. Either can
        /// fail on its own and reports it; a failed one leaves its pass skipped rather than
        /// taking the frame down.
        void initialize(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context);

        void shutdown();

        /// Reads the SSAO and SSR blocks. The rest of the settings belong to other parts of the
        /// renderer, which read them from the same call.
        void setSettings(const EnvironmentSettings &settings);

        /// Drops the occlusion targets, which are sized to a scene target that is going away.
        void releaseTargets();

        /**
         * Measures how much of the sky each pixel can actually see, and takes the rest of the
         * ambient light back out of the frame.
         *
         * @param sceneAmbient the ambient half of what the scene pass shaded, which is the only
         *        part of the frame this may darken - direct light and emission are untouched.
         */
        void drawAmbientOcclusion(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneAmbient,
                                  Diligent::ITexture *sceneDepth, Diligent::ITexture *sceneSurface,
                                  const CameraView &camera, const Matrix &viewProjection);

        /**
         * Marches a reflected ray through the depth buffer and, where it lands on something the
         * frame already contains, swaps the environment's reflection for it.
         *
         * @param ambient the environment the scene pass shaded with, which is what this has to
         *        reproduce in order to subtract it again.
         */
        void drawReflections(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneDepth,
                             Diligent::ITexture *sceneSurface, const CameraView &camera,
                             const Matrix &viewProjection, const AmbientLookup &ambient);

    private:
        /// What the occlusion pass writes and the two passes after it read. One channel because
        /// occlusion is one number, and eight bits because it is then averaged over nine
        /// texels - the sampling noise is orders of magnitude above the quantisation.
        static constexpr Diligent::TEXTURE_FORMAT OCCLUSION_FORMAT = Diligent::TEX_FORMAT_R8_UNORM;

        /// What the occlusion pass turns the depth and surface targets into an occlusion factor
        /// with. Held as values rather than as the parameter block itself: the block belongs to
        /// the project's settings and is handed over by reference per frame.
        struct OcclusionState
        {
            bool enabled = false;
            /// How much of the measured occlusion is applied, and the exponent it is then
            /// shaped by - the two knobs that make this an art direction rather than a
            /// measurement.
            float intensity = 0.0f;
            float power = 1.0f;
            /// How far from a pixel, in world units, the occluders that count can be.
            float radius = 0.0f;
            /// How far behind a sample a surface has to be before it counts as occluding it,
            /// which is what stops a flat surface shadowing itself.
            float bias = 0.0f;
            int sampleCount = 0;
        };

        /// What the reflection pass marches its rays with. Held as values for the same reason
        /// OcclusionState is.
        struct ReflectionState
        {
            bool enabled = false;
            /// How far a ray advances per step, and how far it may travel in total.
            float stepSize = 0.0f;
            float maxDistance = 0.0f;
            /// How far behind the depth buffer a ray may pass and still count as having hit
            /// what is in front of it - the depth buffer records a surface, not a solid, so
            /// this is the thickness it is treated as having.
            float thickness = 0.0f;
            /// How much of the screen's edge a reflection fades out over, as a fraction of it.
            /// A ray that leaves the screen has nothing left to reflect, and fading is what
            /// keeps that from being a visible seam.
            float edgeFade = 0.0f;
            int maxRaySteps = 0;
            /// How many times a hit is bisected between the step that missed and the step that
            /// hit, which is what puts the reflection on the surface rather than a step past it.
            int binarySteps = 0;
        };

        void createOcclusionPipelines();

        void createReflectionPipeline();

        /// Sizes the occlusion targets to @p sceneColor. Nothing announces a resize, so it is
        /// compared rather than trusted.
        void resizeOcclusionTargets(const Diligent::ITexture *sceneColor);

        /// The white texel the reflection pass reads in place of the occlusion target while the
        /// occlusion effect is switched off. A texture rather than a branch because a shader
        /// resource has to be bound either way, and multiplying by one is what "not occluded"
        /// means.
        void createOcclusionFallback();

        /// Borrowed from the renderer, which outlives this and shuts it down before releasing
        /// either.
        Diligent::IRenderDevice *_device = nullptr;
        Diligent::IDeviceContext *_context = nullptr;

        /// Measures the occlusion, blurs the sampling noise out of it, and takes the ambient
        /// light it accounts for back out of the frame. Three pipelines over one constant
        /// buffer: the first two reconstruct the same positions from the same depth, and the
        /// third reads two textures and needs no constants at all.
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _occlusionPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _occlusionBinding;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _occlusionBlurPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _occlusionBlurBinding;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _occlusionApplyPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _occlusionApplyBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _occlusionConstants;
        /// Straight out of the sampling, still carrying the per-pixel rotation as noise.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _occlusionRaw;
        /// The blurred result, which is what the frame is darkened by and what the reflection
        /// pass reads.
        Diligent::RefCntAutoPtr<Diligent::ITexture> _occlusion;
        Diligent::RefCntAutoPtr<Diligent::ITexture> _occlusionFallback;
        /// Which of the two the reflection pass reads: the occlusion measured this frame, or
        /// the white texel that stands for none. Decided by the occlusion pass, because every
        /// way it can give up - switched off, no pipeline, no target - has to leave the pass
        /// after it reading something.
        Diligent::ITexture *_ambientAccess = nullptr;
        OcclusionState _occlusionState{};

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _reflectionPipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _reflectionBinding;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _reflectionConstants;
        ReflectionState _reflectionState{};
    };
} // namespace BreadEngine
