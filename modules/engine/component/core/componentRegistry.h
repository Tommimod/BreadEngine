#pragma once
#include <functional>
#include <ranges>
#include <typeindex>

#include "baseComponentChunk.h"
#include "component.h"

namespace BreadEngine {
    struct RegistryEntry
    {
        RegistryEntry() = default;

        RegistryEntry(const std::type_index ti,
                      std::function<std::unique_ptr<Component>()> compCreator,
                      std::function<std::unique_ptr<BaseComponentChunk>()> chunkCreator)
            : ti(ti), compCreator(std::move(compCreator)), chunkCreator(std::move(chunkCreator))
        {
        }

        ~RegistryEntry() = default;

        std::type_index ti = typeid(nullptr);
        std::function<std::unique_ptr<Component>()> compCreator;
        std::function<std::unique_ptr<BaseComponentChunk>()> chunkCreator;
    };

    class ComponentRegistry
    {
    public:
        static void Register(const std::string &name, std::type_index ti,
                             std::function<std::unique_ptr<Component>()> compCreator,
                             std::function<std::unique_ptr<BaseComponentChunk>()> chunkCreator)
        {
            GetRegistry()[name] = {ti, std::move(compCreator), std::move(chunkCreator)};
        }

        static const RegistryEntry *getEntry(const std::string &name)
        {
            auto &reg = GetRegistry();
            const auto it = reg.find(name);
            if (it != reg.end())
            {
                return &it->second;
            }
            return nullptr;
        }

        static std::vector<std::type_index> getAllComponents()
        {
            auto keys = std::vector<std::type_index>();
            for (auto &registry = GetRegistry(); const auto &key: registry | std::views::keys)
            {
                keys.emplace_back(registry[key].ti);
            }

            return keys;
        }

        static std::string getComponentNameByType(const std::type_index type)
        {
            for (auto &registry = GetRegistry(); const auto &key: registry | std::views::keys)
            {
                if (registry[key].ti == type) return key;
            }

            return "";
        }

    private:
        static std::map<std::string, RegistryEntry> &GetRegistry()
        {
            static std::map<std::string, RegistryEntry> _registry;

            return _registry;
        }
    };
} // BreadEngine
