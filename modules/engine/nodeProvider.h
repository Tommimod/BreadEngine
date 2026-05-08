#pragma once
#include "action.h"

namespace BreadEngine {
    class Node;

    class NodeProvider
    {
    public:
        static Action<Node *> onNodeCreated;
        static Action<Node *> onNodeChangedParent;
        static Action<Node *> onNodeDestroyed;
        static Action<Node *> onNodeRenamed;
        static Action<Node *> onNodeChangedActive;

        static void init();

        [[nodiscard]] static unsigned int generateId();

        [[nodiscard]] static Node *getNode(unsigned int ownerId);

        [[nodiscard]] static const std::vector<Node *> &getAllNodes();

        [[nodiscard]] static Node &createNode();

        static void destroyNode(Node &node);

    private:
        static std::vector<Node *> _nodes;
        static std::vector<unsigned int> _freeIds;
        static unsigned int _id;
    };
} // BreadEngine
