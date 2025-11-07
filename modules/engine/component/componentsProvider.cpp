#include "componentsProvider.h"

namespace BreadEngine {
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentChunk>> ComponentsProvider::_chunks {};
} // BreadEngine