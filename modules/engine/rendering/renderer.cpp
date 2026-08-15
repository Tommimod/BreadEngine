#include "renderer.h"

#include <stdexcept>
#include <string>

#include "logger.h"

#if defined(BREAD_RENDER_BACKEND_DILIGENT)
#include "backends/diligent/diligentRenderer.h"
#else
#include "backends/r3d/r3dRenderer.h"
#endif

namespace BreadEngine {
    std::unique_ptr<IRenderer> Renderer::_instance = nullptr;

    void Renderer::initialize(const int sceneWidth, const int sceneHeight)
    {
        if (_instance != nullptr) return;

#if defined(BREAD_RENDER_BACKEND_DILIGENT)
        _instance = std::make_unique<DiligentRenderer>();
#else
        _instance = std::make_unique<R3DRenderer>();
#endif
        _instance->initialize(sceneWidth, sceneHeight);
        Logger::LogInfo(std::string("Render backend: ") + _instance->getBackendName());
    }

    void Renderer::shutdown()
    {
        if (_instance == nullptr) return;

        _instance->shutdown();
        _instance.reset();
    }

    IRenderer &Renderer::get()
    {
        if (_instance == nullptr)
        {
            constexpr auto message = "Renderer used outside its lifetime - Renderer::initialize() has not run, or shutdown() already has";
            Logger::LogError(message);
            throw std::runtime_error(message);
        }

        return *_instance;
    }
} // namespace BreadEngine
