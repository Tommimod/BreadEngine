#include "nodeNotificator.h"

namespace BreadEngine {
    Action<Node *> NodeNotificator::onNodeCreated {};
    Action<Node *> NodeNotificator::onNodeChangedParent {};
    Action<Node *> NodeNotificator::onNodeDestroyed {};
}