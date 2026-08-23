#pragma once
#include <string>

#include "overlayTypes.h"
#include "renderHandles.h"
#include "renderTypes.h"
#include "geometry/meshData.h"
#include "configs/light/environmentSettings.h"
#include "configs/light/skyboxCubemapParameters.h"
#include "configs/light/skyboxProceduralParameters.h"

namespace BreadEngine {
    /// Everything the engine is allowed to ask of the GPU.
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        [[nodiscard]] virtual const char *getBackendName() const = 0;

        /// @param sceneWidth,sceneHeight initial internal render resolution.
        virtual void initialize(int sceneWidth, int sceneHeight) = 0;

        virtual void shutdown() = 0;

        // --- frame ---

        /**
         * Creates or resizes the offscreen target the scene is rendered into. Callers that
         * never size a target (the game) get the scene on the backbuffer instead.
         * Reallocates render targets, so call it on resize, not per frame.
         */
        virtual void resizeSceneTarget(int width, int height) = 0;

        /**
         * Selects whether the scene is encoded to gamma space on its way to the target.
         * Pushed once the project's settings are known, which is after initialize().
         */
        virtual void setOutputColorSpace(OutputColorSpace colorSpace) = 0;

        /// Opens the scene pass. Draw calls issued until endScene() belong to it.
        virtual void beginScene(const CameraView &camera) = 0;

        /// Renders everything the scene pass collected.
        virtual void endScene() = 0;

        /**
         * The matrix the scene was projected through, valid from beginScene() onward. Its
         * inverse is what turns a pixel of the target back into a ray in the world, so
         * anything picking against what was drawn asks for it here rather than rebuilding it
         * - the aspect and the clip range it was built from are the renderer's own.
         */
        [[nodiscard]] virtual Matrix getViewProjection() const = 0;

        /**
         * Makes the rendered scene the active raylib draw target, so the editor can draw its
         * 3D overlay on top of it and still be occluded by scene geometry. Only meaningful
         * for backends that share a GL context with raylib.
         */
        virtual void beginSceneOverlay() = 0;

        virtual void endSceneOverlay() = 0;

        /// Composites the offscreen scene into @p destination of the current draw target.
        virtual void drawSceneTexture(Rectangle destination) = 0;

        // --- overlay ---

        /**
         * Compiles a shader pair the caller ships into a pipeline the overlay pass can draw
         * through. The renderer never learns what the effect draws: everything specific to
         * that lives in those sources and in the constant block they declare.
         */
        [[nodiscard]] virtual OverlayEffectHandle createOverlayEffect(const OverlayEffectDesc &desc) = 0;

        virtual void destroyOverlayEffect(OverlayEffectHandle handle) = 0;

        [[nodiscard]] virtual OverlayMeshHandle createOverlayMesh(const OverlayMeshData &data) = 0;

        virtual void destroyOverlayMesh(OverlayMeshHandle handle) = 0;

        /**
         * Opens the pass that draws over the finished frame, against the depth the scene was
         * rendered with. Only valid after endScene(), and only drawOverlay() may be called
         * until endOverlay() closes it.
         */
        virtual void beginOverlay() = 0;

        virtual void drawOverlay(const OverlayDrawDesc &draw) = 0;

        virtual void endOverlay() = 0;

        // --- lights ---

        [[nodiscard]] virtual LightHandle createLight(LightType type) = 0;

        virtual void destroyLight(LightHandle handle) = 0;

        [[nodiscard]] virtual bool isLightValid(LightHandle handle) const = 0;

        /// Changing LightState::type recreates the underlying resource; the handle survives it.
        virtual void updateLight(LightHandle handle, const LightState &state) = 0;

        // --- textures ---

        /// Returns immediately; decoding runs in the background and the upload happens on
        /// first use, so asset loading can run ahead of the frame that needs the texture.
        [[nodiscard]] virtual TextureHandle createTexture(const TextureDesc &desc) = 0;

        virtual void destroyTexture(TextureHandle handle) = 0;

        [[nodiscard]] virtual TextureSize getTextureSize(TextureHandle handle) = 0;

        // --- materials ---

        /// Binds @p desc's textures into a surface the draw calls can name by handle.
        [[nodiscard]] virtual MaterialHandle createMaterial(const MaterialDesc &desc) = 0;

        virtual void destroyMaterial(MaterialHandle handle) = 0;

        // --- meshes ---

        /// Uploads geometry the engine has already built, whether generated or imported.
        [[nodiscard]] virtual MeshHandle createMesh(const MeshData &data) = 0;

        virtual void destroyMesh(MeshHandle handle) = 0;

        virtual void drawMesh(const MeshDrawDesc &draw) = 0;

        // --- environment ---

        virtual void setEnvironment(const EnvironmentSettings &settings) = 0;

        /**
         * Returns immediately with a handle whose cube is not filled yet: an environment image
         * is large enough that decoding it on the render thread stalls the frame for seconds.
         * isCubemapReady() says when it has arrived.
         */
        [[nodiscard]] virtual CubemapHandle loadCubemap(const std::string &path,
                                                        const SkyboxCubemapParameters &settings) = 0;

        /// False while a loaded cube is still being decoded, and for a handle naming nothing.
        [[nodiscard]] virtual bool isCubemapReady(CubemapHandle handle) const = 0;

        [[nodiscard]] virtual CubemapHandle createProceduralSky(int size, const SkyboxProceduralParameters &sky) = 0;

        virtual void destroyCubemap(CubemapHandle handle) = 0;

        /// Precomputes the irradiance and reflection maps used for image-based lighting.
        [[nodiscard]] virtual AmbientMapHandle createAmbientMap(CubemapHandle cubemap) = 0;

        virtual void destroyAmbientMap(AmbientMapHandle handle) = 0;
    };
} // namespace BreadEngine
