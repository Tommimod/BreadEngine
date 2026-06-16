#pragma once
#include <memory>
#include <vector>
#include <concepts>

#include "disposeSystem.h"
#include "endFrameSystem.h"
#include "fixedUpdateSystem.h"
#include "initializeSystem.h"
#include "startFrameSystem.h"
#include "updateSystem.h"

#include "standaloneDisposeSystem.h"
#include "standaloneEndFrameSystem.h"
#include "standaloneFixedUpdateSystem.h"
#include "standaloneInitializeSystem.h"
#include "standaloneStartFrameSystem.h"
#include "standaloneUpdateSystem.h"

namespace BreadEngine {
    // Per-node concepts
    template<typename T> concept IsInitializeSystem    = std::derived_from<T, IInitializeSystem>;
    template<typename T> concept IsUpdateSystem        = std::derived_from<T, IUpdateSystem>;
    template<typename T> concept IsFixedUpdateSystem   = std::derived_from<T, IFixedUpdateSystem>;
    template<typename T> concept IsStartFrameSystem    = std::derived_from<T, IStartFrameSystem>;
    template<typename T> concept IsEndFrameSystem      = std::derived_from<T, IEndOfFrameSystem>;
    template<typename T> concept IsDisposeSystem       = std::derived_from<T, IDisposeSystem>;

    // Standalone (once-per-frame) concepts — systems use FilterOption internally for node queries
    template<typename T> concept IsStandaloneInitializeSystem  = std::derived_from<T, IStandaloneInitializeSystem>;
    template<typename T> concept IsStandaloneUpdateSystem      = std::derived_from<T, IStandaloneUpdateSystem>;
    template<typename T> concept IsStandaloneFixedUpdateSystem = std::derived_from<T, IStandaloneFixedUpdateSystem>;
    template<typename T> concept IsStandaloneStartFrameSystem  = std::derived_from<T, IStandaloneStartFrameSystem>;
    template<typename T> concept IsStandaloneEndFrameSystem    = std::derived_from<T, IStandaloneEndOfFrameSystem>;
    template<typename T> concept IsStandaloneDisposeSystem     = std::derived_from<T, IStandaloneDisposeSystem>;

    class SystemsRegistry
    {
    public:
        SystemsRegistry() = default;

        template<typename T>
        SystemsRegistry &addSystem();

        void initialize() const noexcept;
        void update(float deltaTime) const noexcept;
        void fixedUpdate(float fixedDeltaTime) const noexcept;
        void startFrame(float deltaTime) const noexcept;
        void endFrame(float deltaTime) const noexcept;
        void dispose(float deltaTime) const noexcept;

    private:
        std::vector<std::unique_ptr<SystemBase>> _ownedSystems;

        std::vector<IInitializeSystem *>  _initializeSystems;
        std::vector<IUpdateSystem *>      _updateSystems;
        std::vector<IFixedUpdateSystem *> _fixedUpdateSystems;
        std::vector<IStartFrameSystem *>  _startFrameSystems;
        std::vector<IEndOfFrameSystem *>  _endFrameSystems;
        std::vector<IDisposeSystem *>     _disposeSystems;

        std::vector<IStandaloneInitializeSystem *>  _standaloneInitializeSystems;
        std::vector<IStandaloneUpdateSystem *>      _standaloneUpdateSystems;
        std::vector<IStandaloneFixedUpdateSystem *> _standaloneFixedUpdateSystems;
        std::vector<IStandaloneStartFrameSystem *>  _standaloneStartFrameSystems;
        std::vector<IStandaloneEndOfFrameSystem *>  _standaloneEndFrameSystems;
        std::vector<IStandaloneDisposeSystem *>     _standaloneDisposeSystems;
    };

    template<typename T>
    SystemsRegistry &SystemsRegistry::addSystem()
    {
        static_assert(
            IsInitializeSystem<T>          || IsUpdateSystem<T>             || IsFixedUpdateSystem<T>          ||
            IsStartFrameSystem<T>          || IsEndFrameSystem<T>           || IsDisposeSystem<T>               ||
            IsStandaloneInitializeSystem<T> || IsStandaloneUpdateSystem<T>   || IsStandaloneFixedUpdateSystem<T> ||
            IsStandaloneStartFrameSystem<T> || IsStandaloneEndFrameSystem<T> || IsStandaloneDisposeSystem<T>,
            "System T must inherit from one of the per-node or standalone system bases."
        );

        auto system = std::make_unique<T>();
        T *raw = system.get();

        if constexpr (IsInitializeSystem<T>)   _initializeSystems.push_back(raw);
        if constexpr (IsUpdateSystem<T>)        _updateSystems.push_back(raw);
        if constexpr (IsFixedUpdateSystem<T>)   _fixedUpdateSystems.push_back(raw);
        if constexpr (IsStartFrameSystem<T>)    _startFrameSystems.push_back(raw);
        if constexpr (IsEndFrameSystem<T>)      _endFrameSystems.push_back(raw);
        if constexpr (IsDisposeSystem<T>)       _disposeSystems.push_back(raw);

        if constexpr (IsStandaloneInitializeSystem<T>)  _standaloneInitializeSystems.push_back(raw);
        if constexpr (IsStandaloneUpdateSystem<T>)      _standaloneUpdateSystems.push_back(raw);
        if constexpr (IsStandaloneFixedUpdateSystem<T>) _standaloneFixedUpdateSystems.push_back(raw);
        if constexpr (IsStandaloneStartFrameSystem<T>)  _standaloneStartFrameSystems.push_back(raw);
        if constexpr (IsStandaloneEndFrameSystem<T>)    _standaloneEndFrameSystems.push_back(raw);
        if constexpr (IsStandaloneDisposeSystem<T>)     _standaloneDisposeSystems.push_back(raw);

        _ownedSystems.push_back(std::move(system));
        return *this;
    }
} // namespace BreadEngine
