#pragma once
#include <functional>
#include <typeindex>

#include "baseComponentChunk.h"
#include "component.h"

namespace BreadEngine {
    struct RegistryEntry
    {
        RegistryEntry() = default;

        RegistryEntry(std::type_index ti,
                      std::function<std::unique_ptr<Component>()> compCreator,
                      std::function<std::unique_ptr<BaseComponentChunk>()> chunkCreator)
            : ti(std::move(ti)), compCreator(std::move(compCreator)), chunkCreator(std::move(chunkCreator))
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

        static const RegistryEntry *GetEntry(const std::string &name)
        {
            auto &reg = GetRegistry();
            const auto it = reg.find(name);
            if (it != reg.end())
            {
                return &it->second;
            }
            return nullptr;
        }

    private:
        static std::unordered_map<std::string, RegistryEntry> &GetRegistry()
        {
            static std::unordered_map<std::string, RegistryEntry> registry;
            return registry;
        }
    };
} // BreadEngine
