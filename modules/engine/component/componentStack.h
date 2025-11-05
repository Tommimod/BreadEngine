#pragma once
#include <unordered_map>

#include "component.h"

namespace BreadEngine {
    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
    struct ComponentChunk
    {
        ComponentChunk();

        ~ComponentChunk();

        T *get(int ownerId)
        {
        }

        void remove(int ownerId)
        {
        }

    private:
        std::array<T, 1> _defaultProperties;

        T *add(int ownerId)
        {
        }
    };

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> E0>
    ComponentChunk<T, E0>::ComponentChunk() = default;

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> E0>
    ComponentChunk<T, E0>::~ComponentChunk()
    {
    }
} // BreadEngine
