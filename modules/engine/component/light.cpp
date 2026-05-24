#include "light.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Light)
    REGISTER_COMPONENT(Light)

    Light::Light(Node *owner)
    {
        _owner = owner;
    }
} // BreadEngine
