#pragma once
#include "diligentInternal.h"

#include "../resourcePool.h"

namespace BreadEngine {
    /**
     * Draws what a client of the renderer puts over the finished frame, through shaders that
     * client ships itself. The pass owns the pipelines, the geometry and the constant blocks,
     * and learns nothing about what any of them are for: a grid, a manipulator and a widget
     * are the same thing here - vertices, a shader pair and a blob of parameters.
     *
     * It draws into the output texture rather than the scene target, so its colours land
     * already encoded and are neither tone mapped nor post-processed, and it tests against the
     * scene's own depth, so geometry occludes it.
     */
    class OverlayPass
    {
    public:
        void initialize(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context);

        void shutdown();

        [[nodiscard]] OverlayEffectHandle createEffect(const OverlayEffectDesc &desc);

        void destroyEffect(OverlayEffectHandle handle);

        [[nodiscard]] OverlayMeshHandle createMesh(const OverlayMeshData &data);

        void destroyMesh(OverlayMeshHandle handle);

        /// Binds @p output for colour and @p depth for the test, and fills the block every
        /// effect reads. @p viewProjection is the one the scene was drawn with, so the two
        /// agree on where a world point lands.
        void begin(const CameraView &camera, const Matrix &viewProjection,
                   Diligent::ITexture *output, Diligent::ITexture *depth);

        /// @p texture is bound where the effect's pixel shader declares one, and is null
        /// wherever the draw names no texture or the shader reads none.
        void draw(const OverlayDrawDesc &desc, Diligent::ITextureView *texture);

    private:
        /// Mirrors overlay.fxh, and float4-only for the reason the scene's blocks are: that is
        /// the only member layout the struct and the shader cannot drift apart over.
        struct FrameConstants
        {
            float16 viewProjection;
            float16 inverseViewProjection;
            /// xyz is the eye the frame was drawn from; w is unused.
            Vector4 cameraPosition;
        };

        struct EffectSlot
        {
            Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline;
            Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> binding;
            /// The client's own block, or null when its shaders declare none.
            Diligent::RefCntAutoPtr<Diligent::IBuffer> parameters;
            /// Resolved once at creation, so a draw costs a null check rather than a lookup
            /// by name. Null when the pixel shader declares no texture of its own.
            Diligent::IShaderResourceVariable *texture = nullptr;
            Diligent::Uint32 parameterSize = 0;
        };

        struct MeshSlot
        {
            Diligent::RefCntAutoPtr<Diligent::IBuffer> vertices;
            /// Null when the mesh draws its vertices in the order they were given.
            Diligent::RefCntAutoPtr<Diligent::IBuffer> indices;
            Diligent::Uint32 vertexCount = 0;
            Diligent::Uint32 indexCount = 0;
        };

        Diligent::IRenderDevice *_device = nullptr;
        Diligent::IDeviceContext *_context = nullptr;
        /// One block for every effect, filled once per pass rather than per draw: what it
        /// holds is the camera, which no draw of the pass can change.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> _frameConstants;
        ResourcePool<EffectSlot, OverlayEffectHandle> _effects;
        ResourcePool<MeshSlot, OverlayMeshHandle> _meshes;
    };
} // namespace BreadEngine
