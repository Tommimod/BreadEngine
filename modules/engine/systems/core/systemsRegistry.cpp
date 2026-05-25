#include "systemsRegistry.h"

namespace BreadEngine {
    void SystemsRegistry::initialize() const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto &system: _initializeSystems)
            {
                system->initializeInternal(node);
            }
        }
    }

    void SystemsRegistry::update(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            if (!node->getIsActive()) continue;
            for (const auto &system: _updateSystems)
            {
                system->updateInternal(node, deltaTime);
            }
        }
    }

    void SystemsRegistry::fixedUpdate(const float fixedDeltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            if (!node->getIsActive()) continue;
            for (const auto &system: _fixedUpdateSystems)
            {
                system->fixedUpdateInternal(node, fixedDeltaTime);
            }
        }
    }

    void SystemsRegistry::startFrame(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto &system: _startFrameSystems)
            {
                system->startFrameInternal(node, deltaTime);
            }
        }
    }

    void SystemsRegistry::endFrame(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto &system: _endFrameSystems)
            {
                system->endOfFrameInternal(node, deltaTime);
            }
        }
    }

    void SystemsRegistry::dispose(const float deltaTime) const noexcept
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            for (const auto &system: _disposeSystems)
            {
                system->onDisposeInternal(node, deltaTime);
            }
        }
    }
} // BreadEngine
