#pragma once
#include <future>
#include <string>

#include "diligentInternal.h"
#include "../resourcePool.h"

namespace BreadEngine {
    /**
     * The environment a scene is lit by and drawn against: the cube maps it is held in, the
     * passes that bake them, and the two precomputed maps the scene pass shades ambient from.
     *
     * One cube can be reached three ways and they all end in the same texture - baked from
     * Hosek-Wilkie's analytic sky, unwrapped from an equirectangular image, or still being
     * decoded on a worker thread. Precomputing an environment turns it into an irradiance cube
     * and a prefiltered reflection cube; the scene pass reads those alongside the BRDF table,
     * which depends on nothing but the shading model and is integrated once.
     *
     * Nothing here draws into the scene target except the background pass, and that one draws
     * into whatever is already bound.
     */
    class EnvironmentMaps
    {
    public:
        /**
         * Builds the two precompute passes, the cube bound where a scene has no environment,
         * and the BRDF table.
         *
         * Separate from - and before - the sky's own pipelines because the scene pipeline binds
         * that table as a static shader variable, and a static can only be set while no binding
         * has been created against it. The fallback and the table are also built ahead of
         * anything that can fail, so a shader typo is a logged failure rather than a null
         * dereference at the first draw.
         */
        void initializeAmbient(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context);

        /// Builds the two cube map bake passes and the background pass that samples the result.
        /// Nothing here is a static resource of the scene pipeline, so it runs after it.
        void initializeSky();

        void shutdown();

        /// Reads the background and ambient blocks. The rest of the settings belong to other
        /// parts of the renderer, which read them from the same call.
        void setSettings(const EnvironmentSettings &settings);

        /**
         * Returns immediately with a handle whose cube is not filled yet: an environment image
         * is large enough that decoding it on the render thread stalls the frame for seconds.
         *
         * @param encoding the exponent the frame leaves the composite through, which the
         *        authored ground albedo has to be decoded by to reach the scene's linear space.
         */
        [[nodiscard]] CubemapHandle loadCubemap(const std::string &path, const SkyboxCubemapParameters &settings,
                                                float encoding);

        [[nodiscard]] bool isCubemapReady(CubemapHandle handle) const;

        /// Bakes Hosek-Wilkie's model into a cube on the spot; nothing about it has to be
        /// decoded, so it arrives filled. @p encoding decodes the authored colours as above.
        [[nodiscard]] CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky,
                                                        float encoding);

        void destroyCubemap(CubemapHandle handle);

        [[nodiscard]] AmbientMapHandle createAmbientMap(CubemapHandle cubemap);

        void destroyAmbientMap(AmbientMapHandle handle);

        /// Bakes any cube whose image has finished decoding, and frees any slot whose owner let
        /// go mid-decode. Called at the top of the frame, before the scene target is bound,
        /// because baking one binds targets of its own.
        void finalizeCubemaps();

        /// Draws the environment cube behind everything the scene pass rendered, into whatever
        /// target is bound. Writes no depth and draws at the far plane, which is what lets the
        /// passes after it tell background from geometry.
        void drawSkybox(const CameraView &camera, const Matrix &viewProjection);

        /// The flat ambient the scene pass shades with where no environment is bound: rgb the
        /// colour, w the energy. Not decoded into linear, unlike the background - which makes
        /// a solid-colour scene brighter in ambient than the same colour as a sky, and is
        /// deliberate.
        [[nodiscard]] Vector4 ambientColor() const;

        /// Turns a world direction into the environment cube's space, as a quaternion. Already
        /// inverted, because the authored rotation turns the sky and this turns the lookup -
        /// the same inverse the background pass turns its view ray by, so a reflection lands
        /// where the sky it reflects is drawn.
        [[nodiscard]] Vector4 ambientLookupRotation() const;

        /// x is 1 while an environment map is bound, y the highest mip of the reflection cube.
        [[nodiscard]] Vector4 ambientParams() const;

        /// Which environment the scene pass shades ambient from. Nothing announces a rebake,
        /// so a material binding compares against this rather than trusting it.
        [[nodiscard]] AmbientMapHandle ambientMap() const { return _ambientMap; }

        /// What a material binding points its two environment variables at: @p map's own cubes,
        /// or the fallback where it names none. Null only when that fallback failed to build,
        /// which initializeAmbient has already reported.
        [[nodiscard]] Diligent::ITextureView *irradianceView(AmbientMapHandle map) const;

        [[nodiscard]] Diligent::ITextureView *prefilteredView(AmbientMapHandle map) const;

        /// The environment-independent half of the split sum, which the scene pipeline binds
        /// for its lifetime. Null when the integration pass failed.
        [[nodiscard]] Diligent::ITextureView *brdfLutView() const;

    private:
        /// How many mips of the reflection cube are filled, and so how many roughness steps the
        /// scene pass interpolates between. Mip 0 is a mirror and the last is fully rough; below
        /// four levels the steps become visible as bands on a curved surface. The scene pass
        /// reads the top of this range through ambientParams().
        static constexpr Diligent::Uint32 PREFILTERED_CUBE_MIPS = 6;

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
            /// rgb is what the ground reflects. w is whether the equirectangular pass carries
            /// the horizon down over the lower half at all; the analytic dome always fills its
            /// own and ignores w.
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

        /**
         * A cube map, and - while one is still on its way - what it takes to finish it.
         *
         * A procedural sky is baked on the spot and arrives with nothing but its texture. An
         * environment image is decoded off the render thread, so its slot exists and is empty
         * for as long as that takes; a non-null texture is what says it is there.
         */
        struct CubemapSlot
        {
            Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
            /// Decoding produces a loader, which then builds the source texture on the thread
            /// that owns the device. Dropped once the cube has been baked from it.
            Diligent::RefCntAutoPtr<Diligent::ITextureLoader> loader;
            /// The job writes into this slot, so every path that frees or recycles the slot
            /// has to wait on it first - dropping the future does not wait on its own.
            std::future<void> decodeJob;
            /// The ground fill the bake will apply, taken when the load was asked for rather
            /// than when it lands: the settings it came from may have been edited since.
            Diligent::float4 ground{};
            /// Kept for the failure message. The load is asynchronous, so by the time one can
            /// be reported the caller's own path argument is long gone.
            std::string path;
            /// Set when the owner let go while the decode was still writing here. The slot
            /// cannot be recycled under a running job and waiting for one would put back the
            /// freeze that moving the decode off the frame removed, so finalizeCubemaps frees
            /// it on whichever frame the job lands.
            bool abandoned = false;
        };

        /// What one environment precomputes to: the irradiance arriving from every direction
        /// at once, and the reflection of that environment at each roughness, one per mip.
        struct AmbientMapSlot
        {
            Diligent::RefCntAutoPtr<Diligent::ITexture> irradiance;
            Diligent::RefCntAutoPtr<Diligent::ITexture> prefiltered;
        };

        /// Builds the two precompute passes, and the fallbacks ahead of them - those come
        /// first because this can fail and they are what every draw needs regardless.
        void createIblPipelines();

        /// Builds the 1x1 cube bound where a scene has no environment, and the BRDF table.
        void createAmbientFallbacks();

        /// Builds the two cube map bake passes and the background pass that samples them.
        void createSkyPipelines();

        /// Integrates the preintegrated GGX table. Depends on nothing but the shading model,
        /// so it runs once and the pass that fills it is discarded with it.
        void precomputeBrdfLut();

        /**
         * Gives @p slot a cube map of @p size and fills its six faces with @p pipeline,
         * rewriting @p constants' face axes for each. Generates the mip chain, which is what
         * the blur setting samples down. Fills a slot rather than returning one because a
         * loaded cube's slot has to exist before its image does.
         */
        void bakeCubemap(CubemapSlot &slot, const char *name, int size, Diligent::IPipelineState *pipeline,
                         Diligent::IShaderResourceBinding *binding, SkyBakeConstants &constants);

        /**
         * Allocates a cube of @p size with @p mipCount levels and fills every face of every one
         * of them with @p pipeline, rewriting @p constants' face axes and the roughness the mip
         * stands for before each draw.
         */
        [[nodiscard]] Diligent::RefCntAutoPtr<Diligent::ITexture> bakeIblCube(
            const char *name, int size, Diligent::Uint32 mipCount, Diligent::IPipelineState *pipeline,
            Diligent::IShaderResourceBinding *binding, IblBakeConstants &constants);

        /// Borrowed from the renderer, which outlives this and shuts it down before releasing
        /// either.
        Diligent::IRenderDevice *_device = nullptr;
        Diligent::IDeviceContext *_context = nullptr;

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

        ResourcePool<CubemapSlot, CubemapHandle> _cubemaps;
        ResourcePool<AmbientMapSlot, AmbientMapHandle> _ambientMaps;

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
    };
} // namespace BreadEngine
