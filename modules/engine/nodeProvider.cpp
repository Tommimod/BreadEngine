#include "nodeProvider.h"
#include "node.h"
#include "raylib.h"

namespace BreadEngine {
    Action<Node *> NodeProvider::onNodeCreated{};
    Action<Node *> NodeProvider::onNodeChangedParent{};
    Action<Node *> NodeProvider::onNodeDestroyed{};
    Action<Node *> NodeProvider::onNodeRenamed{};
    Action<Node *> NodeProvider::onNodeChangedActive{};

    unsigned int NodeProvider::_id = 0;
    std::vector<Node *> NodeProvider::_nodes{};

    void NodeProvider::init()
    {
        onNodeCreated.subscribe([](Node *node) { _nodes.emplace_back(node); });
        onNodeDestroyed.subscribe([](Node *node) { _nodes.erase(std::ranges::remove(_nodes, node).begin()); });
    }

    unsigned int NodeProvider::generateId()
    {
        return _id++;
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

        TraceLog(LOG_ERROR, "NodeProvider: Can't find node with id %i", ownerId);
        return nullptr;
    }
}
