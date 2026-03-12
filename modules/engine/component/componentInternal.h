#pragma once
#include <typeindex>
#include "componentRegistry.h"
#include "componentChunk.h"

namespace BreadEngine {
#define REGISTER_COMPONENT(ClassName) \
    struct ClassName##_Registrator { \
        ClassName##_Registrator() { \
            BreadEngine::ComponentRegistry::Register( \
                #ClassName, \
                std::type_index(typeid(ClassName)), \
                []() { return std::make_unique<ClassName>(); }, \
                []() { return std::make_unique<BreadEngine::ComponentChunk<ClassName>>(); } \
            ); \
        } \
    }; \
    static ClassName##_Registrator classname##_registrator;
} // BreadEngine
