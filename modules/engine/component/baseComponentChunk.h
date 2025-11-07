#pragma once
#include "component.h"

namespace BreadEngine {
    struct BaseComponentChunk
    {
        virtual ~BaseComponentChunk() = default;

        [[nodiscard]] virtual bool hasOwner(unsigned int ownerId) const = 0;

        [[nodiscard]] virtual Component *getComponent(unsigned int ownerId) = 0;

        [[nodiscard]] virtual bool isEmpty() const = 0;

        virtual void remove(unsigned int ownerId) = 0;
    };
} // BreadEngine
