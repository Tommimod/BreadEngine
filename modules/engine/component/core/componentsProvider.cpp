#include "componentsProvider.h"

namespace BreadEngine {
    bool ComponentsProvider::isDisposed = false;
    Action<unsigned int, std::type_index> ComponentsProvider::onComponentAdded{};
    Action<unsigned int, std::type_index> ComponentsProvider::onComponentRemoved{};
} // BreadEngine
