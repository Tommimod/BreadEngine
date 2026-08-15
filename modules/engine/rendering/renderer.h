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

        /// Falls back to the window size if a caller gets here before Engine::initialize().
        static IRenderer &get();

        [[nodiscard]] static bool isInitialized() { return _instance != nullptr; }

    private:
        static std::unique_ptr<IRenderer> _instance;
    };
} // namespace BreadEngine
