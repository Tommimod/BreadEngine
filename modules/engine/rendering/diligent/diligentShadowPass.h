#pragma once
#include <array>
#include <span>

#include "diligentInternal.h"
// The PI guard again rather than once in diligentInternal.h: that header puts the macro back,
// so a translation unit that reached raylib first is holding it again by the time it gets here.
#pragma push_macro("PI")
#undef PI
#include <Components/interface/ShadowMapManager.hpp>
#pragma pop_macro("PI")

#include "../resourcePool.h"

namespace BreadEngine {
    /// The geometry one fill of a shadow map draws, as the pass borrows it: the frame's draws
    /// and the pool their meshes resolve through. Both belong to the renderer and both are
    /// rewritten between frames, so neither is kept past the call.
    struct ShadowCasters
    {
        std::span<const DrawItem> draws;
        const ResourcePool<MeshSlot, MeshHandle> &meshes;
    };

    /**
     * Every shadow map the scene pass samples, and the depth passes that fill them.
     *
     * Three mechanisms rather than one, because the three light types occlude differently: a
     * directional light gets an array of cascades fitted to the camera's own frustum, a spot
     * gets one slice of a fixed perspective map at the cone's field of view, and an omni light
     * gets a cube whose face is chosen by the direction the surface lies in. What all three
     * share is the depth-only pipeline that fills them and the constant block the scene pass
     * reads them back through.
     *
     * Owns the maps, that pipeline and that block. The casters are handed in per frame, and
     * the buffer a caster's model matrix goes up in is the scene pass's own - the two draw the
     * same geometry and have to agree on where it stands.
     */
    class ShadowPass
    {
    public:
        /**
         * Allocates the three arrays, the comparison sampler they are read through, and the
         * constant buffer the scene pass reads them with.
         *
         * Before the scene pipeline is built: all four are static shader variables of it, and
         * a static can only be set while no binding has been created against it yet.
         */
        void initializeMaps(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context);

        /// Compiles the depth-only shader and builds the one pipeline every map is filled
        /// through. After the scene pipeline, because @p drawConstants is a buffer that
        /// pipeline created - a caster reaches this shader the way it reaches the scene's.
        void initializePipeline(Diligent::IBuffer *drawConstants);

        void shutdown();

        /**
         * Hands out the frame's maps - the cascades to the first directional caster, a slice
         * of the spot array and a cube of the omni one to the next few of each - fills every
         * map that was taken, and uploads what the scene pass samples them with. @p lights is
         * written back through: which map a light was given is what reaches the shader with it.
         *
         * Leaves nothing bound; the scene pass binds its own target afterwards.
         *
         * @param aspect the scene target's, since the cascades are fitted to the frustum the
         *        camera will actually be seen through.
         */
        void render(std::span<VisibleLight> lights, const ShadowCasters &casters, const CameraView &camera,
                    float aspect);

        /// What the scene pipeline binds for its lifetime: the transforms and filter parameters
        /// every map is sampled with, and the three arrays themselves.
        [[nodiscard]] Diligent::IBuffer *constants() const { return _constants; }

        [[nodiscard]] Diligent::ITextureView *cascadeMaps() { return _cascadeMaps.GetSRV(); }

        [[nodiscard]] Diligent::ITextureView *spotMaps() const { return _spotMaps; }

        [[nodiscard]] Diligent::ITextureView *omniMaps() const { return _omniMaps; }

    private:
        /// Mirrors scene.psh's cbuffer of the same name.
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

        /// Allocates one depth array of @p sliceDSVs.size() slices, the view the scene pass
        /// samples it through, and one depth-stencil view per slice for the passes that fill it.
        void createArray(const char *name, Diligent::RESOURCE_DIMENSION dimension, Diligent::Uint32 resolution,
                         Diligent::ISampler *comparisonSampler,
                         Diligent::RefCntAutoPtr<Diligent::ITextureView> &srv,
                         std::span<Diligent::RefCntAutoPtr<Diligent::ITextureView>> sliceDSVs);

        /// Gives each of @p lights the map its type calls for, as far as the arrays reach.
        void assignSlots(std::span<VisibleLight> lights);

        /// Draws every casting item of @p casters into @p target, seen through
        /// @p worldToLightClip. Already in upload order, because its two callers arrive at it
        /// from different places - one from raylib's math, one out of DiligentFX.
        void renderCasters(Diligent::ITextureView *target, const float16 &worldToLightClip,
                           const ShadowCasters &casters);

        /// Fits and fills the cascade array for one directional light.
        void renderCascades(const LightState &light, const ShadowCasters &casters, const CameraView &camera,
                            float aspect);

        /// Fills one slice of the spot array, and records the transform to sample it with.
        void renderSpotMap(const LightState &light, int slice, const ShadowCasters &casters);

        /// Fills all six faces of one cube of the omni array, and records what the scene pass
        /// rebuilds their depth with.
        void renderOmniMap(const LightState &light, int slice, const ShadowCasters &casters);

        /// Borrowed from the renderer, which outlives this and shuts it down before releasing
        /// any of them.
        Diligent::IRenderDevice *_device = nullptr;
        Diligent::IDeviceContext *_context = nullptr;
        Diligent::IBuffer *_drawConstants = nullptr;

        /// One directional light's cascades, refitted to the camera every frame by DiligentFX.
        Diligent::ShadowMapManager _cascadeMaps;
        /// The spot lights' maps, one array slice each. Not the cascade manager's job: a spot
        /// needs a single perspective map, not a set fitted to the camera's frustum.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> _spotMaps;
        std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, MAX_SPOT_SHADOWS> _spotDSVs;
        /// The omni lights' maps, one cube each. A cube rather than six flat slices so the
        /// scene pass picks the face from the direction it is already holding.
        Diligent::RefCntAutoPtr<Diligent::ITextureView> _omniMaps;
        std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, MAX_OMNI_SHADOWS * CUBE_FACE_COUNT> _omniDSVs;

        Diligent::RefCntAutoPtr<Diligent::IPipelineState> _pipeline;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _binding;
        /// The map currently being filled, as the one transform it is seen through. Rewritten
        /// per cascade and per cube face.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _passConstants;
        /// What the scene pass reads shadowing from, and the copy filled in over the frame.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _constants;
        ShadowConstants _data{};
    };
} // namespace BreadEngine
