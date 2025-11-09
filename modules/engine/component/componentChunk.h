#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include "baseComponentChunk.h"
#include "component.h"
#include "raylib.h"
#include "../nodeProvider.h"

namespace BreadEngine {
    class NodeProvider;

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> >
    struct ComponentChunk final : BaseComponentChunk
    {
        ComponentChunk() = default;

        ~ComponentChunk() override = default;

        T &get(unsigned int ownerId);

        [[nodiscard]] bool isEmpty() const override
        {
            return _ownerIdToIndex.empty();
        }

        [[nodiscard]] bool hasOwner(const unsigned int ownerId) const override
        {
            return _ownerIdToIndex.contains(ownerId);
        }

        [[nodiscard]] Component *getComponent(const unsigned int ownerId) override
        {
            const auto it = _ownerIdToIndex.find(ownerId);
            if (it == _ownerIdToIndex.end())
            {
                return nullptr;
            }

            return static_cast<Component *>(&_components[it->second]);
        }

        T &add(const unsigned int ownerId, T component)
        {
            if (_ownerIdToIndex.contains(ownerId))
            {
                TraceLog(LOG_ERROR, "Component %s already exists for node &i", typeid(T).name(), ownerId);
                return _components[_ownerIdToIndex[ownerId]];
            }

            tryResize();
            if (!_freeSlots.empty())
            {
                const int index = _freeSlots.front();
                _freeSlots.erase(_freeSlots.begin());
                _ownerIds[index] = ownerId;
                _ownerIdToIndex.emplace(ownerId, index);
                _components[index] = std::move(component);
                return _components[index];
            }

            _ownerIds[_size] = ownerId;
            _components[_size] = std::move(component);
            _ownerIdToIndex.emplace(ownerId, _size);
            return _components[_size++];
        }

        void remove(const unsigned int ownerId) override
        {
            if (!_ownerIdToIndex.contains(ownerId))
            {
                return;
            }

            const int id = _ownerIdToIndex[ownerId];
            _freeSlots.push_back(id);
            _ownerIds[id] = -1;
            _components[id].destroy();
            _components[id] = T{};
            _ownerIdToIndex.erase(ownerId);
        }

    private:
        int _size = 0;
        std::vector<T> _components{};
        std::vector<unsigned int> _ownerIds{};
        std::vector<int> _freeSlots{};
        std::unordered_map<unsigned int, int> _ownerIdToIndex{};

        T &add(unsigned int ownerId);

        void tryResize();
    };
} // BreadEngine

namespace BreadEngine {
    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> E0>
    T &ComponentChunk<T, E0>::get(const unsigned int ownerId)
    {
        if (_ownerIdToIndex.contains(ownerId))
        {
            return _components[_ownerIdToIndex[ownerId]];
        }

        return add(ownerId);
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> E0>
    T &ComponentChunk<T, E0>::add(const unsigned int ownerId)
    {
        tryResize();
        if (!_freeSlots.empty())
        {
            const int index = _freeSlots.front();
            _freeSlots.erase(_freeSlots.begin());
            _ownerIds[index] = ownerId;
            _ownerIdToIndex.emplace(ownerId, index);
            auto c = &_components[index];
            c->setOwner(NodeProvider::getNode(ownerId));
            c->init();
            return _components[index];
        }

        _ownerIds[_size] = ownerId;
        _ownerIdToIndex.emplace(ownerId, _size);
        auto c = &_components[_size];
        c->setOwner(NodeProvider::getNode(ownerId));
        c->init();
        return _components[_size++];
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> E0>
    void ComponentChunk<T, E0>::tryResize()
    {
        if (_components.empty())
        {
            constexpr int capacity = 16;
            _components.resize(capacity, T{});
            _ownerIds.resize(capacity, -1);
            _freeSlots.reserve(capacity);
            for (int i = 0; i < capacity; i++)
            {
                _freeSlots.push_back(i);
            }
        }

        if (_freeSlots.empty() && _size == _components.capacity())
        {
            _components.resize(_components.capacity() * 2, T{});
            _ownerIds.resize(_ownerIds.capacity() * 2, -1);
        }
    }
}
