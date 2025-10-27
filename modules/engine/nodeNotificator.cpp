#include "nodeNotificator.h"

namespace BreadEngine {
    Action<Node *> NodeNotificator::onNodeCreated {};
    Action<Node *> NodeNotificator::onNodeChangedParent {};
    Action<Node *> NodeNotificator::onNodeDestroyed {};
    Action<Node *> NodeNotificator::onNodeRenamed {};
    Action<Node *> NodeNotificator::onNodeChangedActive {};
}