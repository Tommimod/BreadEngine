#pragma once
#include "rendering/renderHandles.h"

namespace BreadEditor {
    /**
     * The ground plane the viewport is laid out over. Owns the shader pair the editor ships
     * for it and the one triangle it is drawn as - the renderer knows only that much, and the
     * grid itself is entirely in grid.psh and in the parameters handed to it.
     */
    class GridRenderer
    {
    public:
        void initialize();

        void shutdown();

        /// Draws into the open overlay pass, so it belongs between beginOverlay and endOverlay.
        void render() const;

    private:
        BreadEngine::OverlayEffectHandle _effect{};
        BreadEngine::OverlayMeshHandle _mesh{};
    };
} // namespace BreadEditor
