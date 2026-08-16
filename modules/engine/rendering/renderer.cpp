#include "renderer.h"

#include <stdexcept>
#include <string>

#include "logger.h"

#include "diligent/diligentRenderer.h"

namespace BreadEngine {
    std::unique_ptr<IRenderer> Renderer::_instance = nullptr;

    void Renderer::initialize(const int sceneWidth, const int sceneHeight)
    {
        if (_instance != nullptr) return;

        _instance = std::make_unique<DiligentRenderer>();
        _instance->initialize(sceneWidth, sceneHeight);
        Logger::LogInfo(std::string("Render backend: ") + _instance->getBackendName());
    }

    void Renderer::shutdown()
    {
        if (_instance == nullptr) return;

        _instance->shutdown();
        _instance.reset();
    }

    bool Renderer::isAlive()
    {
        return _instance != nullptr;
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
