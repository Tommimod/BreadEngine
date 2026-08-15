#pragma once
#include <memory>
#include "IRenderer.h"

namespace BreadEngine {
    /**
     * Owns the process-wide IRenderer instance and hides which backend was compiled in.
     * Everything in the engine, editor and game reaches the renderer through Renderer::get().
     */
    class Renderer
    {
    public:
        static void initialize(int sceneWidth, int sceneHeight);

        static void shutdown();

        /// Throws when called before initialize() or after shutdown() - reaching the GPU
        /// outside the renderer's lifetime is a call-order bug, not something to paper over.
        static IRenderer &get();

    private:
        static std::unique_ptr<IRenderer> _instance;
    };
} // namespace BreadEngine
