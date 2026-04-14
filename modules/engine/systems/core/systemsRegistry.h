#pragma once
#include <memory>
#include <vector>
#include <concepts>

#include "endFrameSystem.h"
#include "fixedUpdateSystem.h"
#include "initializeSystem.h"
#include "startFrameSystem.h"
#include "updateSystem.h"

namespace BreadEngine {
    template<typename T>
    concept IsInitializeSystem = std::derived_from<T, InitializeSystem>;

    template<typename T>
    concept IsUpdateSystem = std::derived_from<T, UpdateSystem>;

    template<typename T>
    concept IsFixedUpdateSystem = std::derived_from<T, FixedUpdateSystem>;

    template<typename T>
    concept IsStartFrameSystem = std::derived_from<T, StartFrameSystem>;

    template<typename T>
    concept IsEndFrameSystem = std::derived_from<T, EndOfFrameSystem>;

    class SystemsRegistry
    {
    public:
        SystemsRegistry() = default;

        ~SystemsRegistry()
        {
            _initializeSystems.clear();
            _updateSystems.clear();
            _fixedUpdateSystems.clear();
            _startFrameSystems.clear();
            _endFrameSystems.clear();
        }

        template<typename T>
        SystemsRegistry &addSystem();

        void initialize() const noexcept;

        void update(float deltaTime) const noexcept;

        void fixedUpdate(float fixedDeltaTime) const noexcept;

        void startFrame(float deltaTime) const noexcept;

        void endFrame(float deltaTime) const noexcept;

    private:
        std::vector<std::shared_ptr<InitializeSystem> > _initializeSystems{};
        std::vector<std::shared_ptr<UpdateSystem> > _updateSystems{};
        std::vector<std::shared_ptr<FixedUpdateSystem> > _fixedUpdateSystems{};
        std::vector<std::shared_ptr<StartFrameSystem> > _startFrameSystems{};
        std::vector<std::shared_ptr<EndOfFrameSystem> > _endFrameSystems{};
    };

    template<typename T>
    SystemsRegistry &SystemsRegistry::addSystem()
    {
        static_assert(
            IsInitializeSystem<T> || IsUpdateSystem<T> || IsFixedUpdateSystem<T> || IsStartFrameSystem<T> || IsEndFrameSystem<T>,
            "System T must publicly inherit from one of: "
            "InitializeSystem, UpdateSystem, FixedUpdateSystem, StartFrameSystem or EndOfFrameSystem"
        );

        auto system = std::make_shared<T>();
        if constexpr (IsInitializeSystem<T>) _initializeSystems.emplace_back(system);
        if constexpr (IsUpdateSystem<T>) _updateSystems.emplace_back(system);
        if constexpr (IsFixedUpdateSystem<T>) _fixedUpdateSystems.emplace_back(system);
        if constexpr (IsStartFrameSystem<T>) _startFrameSystems.emplace_back(system);
        if constexpr (IsEndFrameSystem<T>) _endFrameSystems.emplace_back(system);

        return *this;
    }
} // BreadEngine
