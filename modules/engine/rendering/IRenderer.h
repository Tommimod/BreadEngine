#pragma once
#include "renderHandles.h"
#include "renderTypes.h"

namespace BreadEngine {
    /**
     * The backend seam for all GPU work (DILIGENT_MIGRATION.md, Phase 2).
     *
     * Exactly one implementation is compiled and linked at a time, selected by the
     * BREAD_RENDER_BACKEND CMake option. The interface is virtual so that both
     * implementations can live in the tree while the migration is in progress and so a
     * single call site reads the same under either backend - not because backends are ever
     * swapped at runtime.
     *
     * Methods are grouped by the migration sub-phase that introduces them; the interface
     * grows one group at a time rather than being declared up front against r3d's shape.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        [[nodiscard]] virtual const char *getBackendName() const = 0;

        virtual void initialize() = 0;

        virtual void shutdown() = 0;

        // --- lights (2.a) ---

        [[nodiscard]] virtual LightHandle createLight(LightType type) = 0;

        virtual void destroyLight(LightHandle handle) = 0;

        [[nodiscard]] virtual bool isLightValid(LightHandle handle) const = 0;

        /**
         * Applies @p state to the light. Changing LightState::type recreates the underlying
         * backend resource in place, so the handle stays valid across a type change.
         */
        virtual void updateLight(LightHandle handle, const LightState &state) = 0;
    };
} // namespace BreadEngine
