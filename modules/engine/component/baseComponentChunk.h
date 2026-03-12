#pragma once

namespace BreadEngine {
    struct Component;

    struct BaseComponentChunk
    {
        virtual ~BaseComponentChunk() = default;

    private:
        friend class ComponentsProvider;

        virtual void addComponent(unsigned int ownerId, std::unique_ptr<Component> comp, bool override = false) = 0;

        [[nodiscard]] virtual bool hasOwner(unsigned int ownerId) const = 0;

        [[nodiscard]] virtual Component *getComponent(unsigned int ownerId) = 0;

        [[nodiscard]] virtual bool isEmpty() const = 0;

        virtual void remove(unsigned int ownerId) = 0;
    };
} // BreadEngine
