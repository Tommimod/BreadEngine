#include "nodeProvider.h"
#include "node.h"
#include "objectPool.h"
#include "logger.h"
#include "raylib.h"
#include "tracy/Tracy.hpp"
#include <algorithm>

namespace BreadEngine {
    auto nodeFactory = []() -> Node *
    {
        return new Node();
    };
    ObjectPool<Node> nodePool(nodeFactory, 10);
    Action<Node *> NodeProvider::onNodeCreated{};
    Action<Node *> NodeProvider::onNodeChangedParent{};
    Action<Node *> NodeProvider::onNodeDestroyed{};
    Action<Node *> NodeProvider::onNodeRenamed{};
    Action<Node *> NodeProvider::onNodeChangedActive{};

    unsigned int NodeProvider::_id = 0;
    std::vector<Node *> NodeProvider::_nodes{};
    std::vector<unsigned int> NodeProvider::_freeIds{};

    void NodeProvider::init()
    {
        ZoneScoped;
        onNodeCreated.subscribe([](Node *node) { _nodes.emplace_back(node); });
        onNodeDestroyed.subscribe([](Node *node)
        {
            _nodes.erase(std::ranges::remove(_nodes, node).begin());
            _freeIds.emplace_back(node->getId());
        });
    }

    unsigned int NodeProvider::generateId()
    {
        ZoneScoped;
        if (!_freeIds.empty())
        {
            const auto nextId = _freeIds[_freeIds.size() - 1];
            _freeIds.pop_back();
            return nextId;
        }

        return ++_id;
    }

    Node *NodeProvider::getNode(const unsigned int ownerId)
    {
        ZoneScoped;
        for (const auto node: _nodes)
        {
            if (node->_id == ownerId)
            {
                return node;
            }
        }

        return nullptr;
    }

    const std::vector<Node *> &NodeProvider::getAllNodes()
    {
        return _nodes;
    }

    Node &NodeProvider::createNode()
    {
        auto &node = nodePool.get();
        return node;
    }

    Node &NodeProvider::createNode(const unsigned int id)
    {
        auto &node = nodePool.get();
        assignId(node, id);
        return node;
    }

    void NodeProvider::assignId(Node &node, const unsigned int id)
    {
        ZoneScoped;
        if (const auto *existing = getNode(id); existing != nullptr && existing != &node)
        {
            Logger::LogWarning(TextFormat("NodeProvider: id %u is already assigned to another live node, overriding could cause conflicts", id));
        }

        if (const auto it = std::ranges::find(_freeIds, id); it != _freeIds.end())
        {
            _freeIds.erase(it);
        }

        if (id > _id)
        {
            _id = id;
        }

        node._id = id;
    }

    void NodeProvider::destroyNode(Node &node)
    {
        nodePool.release(node);
    }
}
