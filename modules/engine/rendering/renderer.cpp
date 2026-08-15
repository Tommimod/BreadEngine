#include "renderer.h"

#include <string>

#include "logger.h"
#include "raylib.h"

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
        // Function-local static, not TextFormat(): Logger stores the std::string_view it is
        // handed, so the storage behind it has to outlive the call.
        static const std::string message = std::string("Render backend: ") + _instance->getBackendName();
        Logger::LogInfo(message);
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
            initialize(GetScreenWidth(), GetScreenHeight());
        }
        return *_instance;
    }
} // namespace BreadEngine
