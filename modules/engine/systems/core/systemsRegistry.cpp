#include "systemsRegistry.h"

namespace BreadEngine {
    void SystemsRegistry::initialize() const noexcept
    {
        for (const auto &system: _initializeSystems)
        {
            system->initializeInternal();
        }
    }

    void SystemsRegistry::update(const float deltaTime) const noexcept
    {
        for (const auto &system: _updateSystems)
        {
            system->updateInternal(deltaTime);
        }
    }

    void SystemsRegistry::fixedUpdate(const float fixedDeltaTime) const noexcept
    {
        for (const auto &system: _fixedUpdateSystems)
        {
            system->fixedUpdateInternal(fixedDeltaTime);
        }
    }

    void SystemsRegistry::startFrame(const float deltaTime) const noexcept
    {
        for (const auto &system: _startFrameSystems)
        {
            system->startFrameInternal(deltaTime);
        }
    }

    void SystemsRegistry::endFrame(const float deltaTime) const noexcept
    {
        for (const auto &system: _endFrameSystems)
        {
            system->endOfFrameInternal(deltaTime);
        }
    }
} // BreadEngine
