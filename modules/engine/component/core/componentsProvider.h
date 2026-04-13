#pragma once
#include <any>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "baseComponentChunk.h"
#include "componentChunk.h"
#include "componentRegistry.h"

namespace BreadEngine {
    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> >
    struct ComponentChunk;

    class ComponentsProvider
    {
    public:
        static constexpr size_t MAX_COMPONENTS = 512;
        static constexpr size_t MAX_MASK_COUNT = (MAX_COMPONENTS + 63) / 64;
        using ComponentMaskArray = std::array<uint64_t, MAX_MASK_COUNT>;

        static ComponentMaskArray GetComponentMasks(const unsigned int ownerId, const int maskCount)
        {
            ComponentMaskArray masks{};

            if (maskCount > static_cast<int>(MAX_MASK_COUNT))
            {
                throw std::runtime_error("maskCount is low. Increase MAX_MASK_COUNT.");
            }

            for (const auto &[ti, chunkPtr]: getChunks())
            {
                if (chunkPtr->hasOwner(ownerId))
                {
                    const size_t compID = GetComponentID(ti);
                    const size_t arrayIndex = compID / 64;
                    const size_t bitIndex = compID % 64;

                    if (arrayIndex < static_cast<size_t>(maskCount))
                    {
                        masks[arrayIndex] |= (1ULL << bitIndex);
                    }
                }
            }

            return masks;
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static size_t GetComponentIndex()
        {
            return GetComponentID(std::type_index(typeid(T)));
        }

        static void addDynamic(const unsigned int ownerId, const std::string &componentType)
        {
            const auto *entry = ComponentRegistry::getEntry(componentType);
            if (!entry)
            {
                throw std::runtime_error("Unable to add dynamic component");
            }

            auto comp = entry->compCreator();
            comp->setOwner(NodeProvider::getNode(ownerId));

            auto &chunks = getChunks();
            auto ti = entry->ti;
            auto it = chunks.find(ti);
            if (it == chunks.end())
            {
                chunks.emplace(ti, entry->chunkCreator());
                it = chunks.find(ti);
            }

            auto &baseChunk = *it->second;
            baseChunk.addComponent(ownerId, std::move(comp), true);
        }

        static void addDynamic(const unsigned int ownerId, const std::string &componentType, const YAML::Node &node)
        {
            const auto *entry = ComponentRegistry::getEntry(componentType);
            if (!entry)
            {
                throw std::runtime_error("Unable to add dynamic component");
            }

            auto comp = entry->compCreator();
            comp->deserialize(node);

            auto &chunks = getChunks();
            auto ti = entry->ti;
            auto it = chunks.find(ti);
            if (it == chunks.end())
            {
                chunks.emplace(ti, entry->chunkCreator());
                it = chunks.find(ti);
            }

            auto &baseChunk = *it->second;
            baseChunk.addComponent(ownerId, std::move(comp), true);
        }

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
            const auto it = getChunks().find(ti);
            if (it == getChunks().end())
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
            if (const auto it = getChunks().find(ti); it != getChunks().end())
            {
                auto &baseChunk = *it->second;
                auto &chunk = dynamic_cast<ComponentChunk<T> &>(baseChunk);
                chunk.remove(ownerId);
                if (chunk.isEmpty())
                {
                    getChunks().erase(ti);
                }
            }
        }

        static void remove(const unsigned int ownerId, const std::type_index type)
        {
            if (const auto it = getChunks().find(type); it != getChunks().end())
            {
                auto &baseChunk = *it->second;
                baseChunk.remove(ownerId);
                if (baseChunk.isEmpty())
                {
                    getChunks().erase(type);
                }
            }
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static bool has(unsigned int ownerId)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
            const auto ti = std::type_index(typeid(T));
            const auto it = getChunks().find(ti);
            if (it == getChunks().end())
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
            for (const auto &p: getChunks())
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
            for (const auto &p: getChunks())
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
            if (isDisposed)
            {
                return;
            }

            for (const auto &val: getChunks() | std::views::values)
            {
                val->remove(ownerId);
            }
        }

        static void setDisposed() { isDisposed = true; }

    private:
        static bool isDisposed;

        static std::unordered_map<std::type_index, std::unique_ptr<BaseComponentChunk> > &getChunks() noexcept
        {
            static std::unordered_map<std::type_index, std::unique_ptr<BaseComponentChunk> > chunks{};
            return chunks;
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static ComponentChunk<T> &emplaceChunk()
        {
            auto ti = std::type_index(typeid(T));
            getChunks().emplace(ti, std::make_unique<ComponentChunk<T> >());
            return dynamic_cast<ComponentChunk<T> &>(*getChunks().at(ti));
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        static T &addImpl(unsigned int ownerId, T component)
        {
            auto &chunk = emplaceChunk<T>();
            return chunk.add(ownerId, std::move(component), false);
        }

        static size_t &GetNextComponentID() noexcept
        {
            static size_t counter = 0;
            return counter;
        }

        static std::unordered_map<std::type_index, size_t> &getComponentIDs() noexcept
        {
            static std::unordered_map<std::type_index, size_t> ids{};
            return ids;
        }

        static size_t GetComponentID(const std::type_index &ti)
        {
            auto &map = getComponentIDs();
            if (const auto it = map.find(ti); it != map.end()) return it->second;

            const size_t id = GetNextComponentID()++;
            map[ti] = id;
            return id;
        }
    };
} // BreadEngine
