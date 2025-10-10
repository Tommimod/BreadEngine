#pragma once
#include "action.h"
#include "node.h"

namespace BreadEngine {
    class NodeNotificator
    {
    public:
        static Action<Node *> onNodeCreated;
        static Action<Node *> onNodeChangedParent;
        static Action<Node *> onNodeDestroyed;
    };
} // BreadEngine
