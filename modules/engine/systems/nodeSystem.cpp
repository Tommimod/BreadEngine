#include "nodeSystem.h"

namespace BreadEngine {
    void NodeSystem::initialize(const std::vector<Node *> &nodes)
    {
        for (const auto *node: nodes)
        {
            for (auto components = ComponentsProvider::getAllComponents(node->getId()); auto *component: components)
            {
                component->initialize();
            }
        }
    }

    void NodeSystem::update(const std::vector<Node *> &nodes, const float deltaTime)
    {
        for (const auto *node: nodes)
        {
            for (auto components = ComponentsProvider::getAllComponents(node->getId()); auto *component: components)
            {
                component->update(deltaTime);
            }
        }
    }

    void NodeSystem::fixedUpdate(const std::vector<Node *> &nodes, const float fixedDeltaTime)
    {
        for (const auto *node: nodes)
        {
            for (auto components = ComponentsProvider::getAllComponents(node->getId()); auto *component: components)
            {
                component->fixedUpdate(fixedDeltaTime);
            }
        }
    }

    void NodeSystem::startFrame(const std::vector<Node *> &nodes, const float deltaTime)
    {
        for (const auto *node: nodes)
        {
            for (auto components = ComponentsProvider::getAllComponents(node->getId()); auto *component: components)
            {
                component->onFrameStart(deltaTime);
            }
        }
    }

    void NodeSystem::endOnFrame(const std::vector<Node *> &nodes, const float deltaTime)
    {
        for (const auto *node: nodes)
        {
            for (auto components = ComponentsProvider::getAllComponents(node->getId()); auto *component: components)
            {
                component->onFrameEnd(deltaTime);
            }
        }
    }
} // BreadEngine
