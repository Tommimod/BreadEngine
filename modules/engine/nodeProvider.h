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

        static unsigned int generateId();

        static Node *getNode(unsigned int ownerId);

    private:
        static unsigned int _id;
        static std::vector<Node *> _nodes;
    };
} // BreadEngine
