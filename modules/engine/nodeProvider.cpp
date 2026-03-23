#include "nodeProvider.h"
#include "node.h"

namespace BreadEngine {
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
        onNodeCreated.subscribe([](Node *node) { _nodes.emplace_back(node); });
        onNodeDestroyed.subscribe([](Node *node)
        {
            _nodes.erase(std::ranges::remove(_nodes, node).begin());
            _freeIds.emplace_back(node->getId());
        });
    }

    unsigned int NodeProvider::generateId()
    {
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
}
