#include "systemsRegistry.h"

namespace BreadEngine {
    void SystemsRegistry::initialize() const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto system: _initializeSystems)
                system->initializeInternal(node);
        }
        for (const auto system: _standaloneInitializeSystems)
            system->standaloneInitializeInternal();
    }

    void SystemsRegistry::update(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            if (!node->getIsActive()) continue;
            for (const auto system: _updateSystems)
                system->updateInternal(node, deltaTime);
        }
        for (const auto system: _standaloneUpdateSystems)
            system->standaloneUpdateInternal(deltaTime);
    }

    void SystemsRegistry::fixedUpdate(const float fixedDeltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            if (!node->getIsActive()) continue;
            for (const auto system: _fixedUpdateSystems)
                system->fixedUpdateInternal(node, fixedDeltaTime);
        }
        for (const auto system: _standaloneFixedUpdateSystems)
            system->standaloneFixedUpdateInternal(fixedDeltaTime);
    }

    void SystemsRegistry::startFrame(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto system: _startFrameSystems)
                system->startFrameInternal(node, deltaTime);
        }
        for (const auto system: _standaloneStartFrameSystems)
            system->standaloneStartFrameInternal(deltaTime);
    }

    void SystemsRegistry::endFrame(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto system: _endFrameSystems)
                system->endOfFrameInternal(node, deltaTime);
        }
        for (const auto system: _standaloneEndFrameSystems)
            system->standaloneEndOfFrameInternal(deltaTime);
    }

    void SystemsRegistry::dispose(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto system: _disposeSystems)
                system->onDisposeInternal(node, deltaTime);
        }
        for (const auto system: _standaloneDisposeSystems)
            system->standaloneOnDisposeInternal(deltaTime);
    }
} // namespace BreadEngine
