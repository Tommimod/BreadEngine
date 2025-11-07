#pragma once
#include <any>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "baseComponentChunk.h"
#include "componentChunk.h"

namespace BreadEngine {
    //forward declaration for ComponentChunk<T>
    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
    struct ComponentChunk;

    class ComponentsProvider
    {
    public:
        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static T &add(const unsigned int ownerId)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
            return addImpl<T>(ownerId, T{});
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static T &add(const unsigned int ownerId, T component)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
            return addImpl<T>(ownerId, std::move(component));
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static T &get(unsigned int ownerId)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
            const auto ti = std::type_index(typeid(T));
            const auto it = _chunks.find(ti);
            if (it == _chunks.end())
            {
                auto &chunk = emplaceChunk<T>();
                return add<T>(ownerId);
            }

            auto &baseChunk = *it->second;
            auto &chunk = dynamic_cast<ComponentChunk<T> &>(baseChunk);
            return chunk.get(ownerId);
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static void remove(unsigned int ownerId)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
            const auto ti = std::type_index(typeid(T));
            if (const auto it = _chunks.find(ti); it != _chunks.end())
            {
                auto &baseChunk = *it->second;
                auto &chunk = dynamic_cast<ComponentChunk<T> &>(baseChunk);
                chunk.remove(ownerId);
                if (chunk.isEmpty())
                {
                    _chunks.erase(ti);
                }
            }
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static bool has(unsigned int ownerId)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
            const auto ti = std::type_index(typeid(T));
            const auto it = _chunks.find(ti);
            if (it == _chunks.end())
            {
                return false;
            }

            auto &baseChunk = *it->second;
            auto &chunk = dynamic_cast<ComponentChunk<T> &>(baseChunk);
            return chunk.hasOwner(ownerId);
        }

        static std::vector<std::type_index> getAllComponentTypes(const unsigned int ownerId)
        {
            std::vector<std::type_index> types;
            for (const auto &p: _chunks)
            {
                if (p.second->hasOwner(ownerId))
                {
                    types.push_back(p.first);
                }
            }

            return types;
        }

        static std::vector<Component *> getAllComponents(const unsigned int ownerId)
        {
            std::vector<Component *> components;
            for (const auto &p: _chunks)
            {
                if (auto *comp = p.second->getComponent(ownerId))
                {
                    components.push_back(comp);
                }
            }

            return components;
        }

        static void clearOwner(const unsigned int ownerId)
        {
            for (auto &p: _chunks)
            {
                p.second->remove(ownerId);
            }
        }

    private:
        static std::unordered_map<std::type_index, std::unique_ptr<BaseComponentChunk> > _chunks;

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static ComponentChunk<T> &emplaceChunk()
        {
            auto ti = std::type_index(typeid(T));
            _chunks.emplace(ti, std::make_unique<ComponentChunk<T> >());
            return dynamic_cast<ComponentChunk<T> &>(*_chunks.at(ti));
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static T &addImpl(unsigned int ownerId, T component)
        {
            auto &chunk = emplaceChunk<T>();
            return chunk.add(ownerId, std::move(component));
        }
    };
} // BreadEngine
