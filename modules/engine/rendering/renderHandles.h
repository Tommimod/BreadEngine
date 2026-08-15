#pragma once
#include <cstdint>

namespace BreadEngine {
    /**
     * Opaque, backend-agnostic reference to a resource owned by the active IRenderer.
     *
     * Deliberately a plain value type: only one backend is alive at a time (see
     * BREAD_RENDER_BACKEND), so a handle only has to be unambiguous inside that backend's own
     * pool - there is nothing to dispatch on, and components stay copyable/trivially
     * serializable-around. `generation` is bumped when a slot is reused, so a handle kept
     * across a destroy (a stale handle) resolves to nothing instead of silently addressing
     * whatever resource landed in that slot next.
     *
     * The Tag parameter only exists to keep the handle families from implicitly converting
     * into each other.
     */
    template<typename Tag>
    struct RenderHandle
    {
        static constexpr uint32_t INVALID_INDEX = UINT32_MAX;

        uint32_t index = INVALID_INDEX;
        uint32_t generation = 0;

        [[nodiscard]] bool isValid() const { return index != INVALID_INDEX; }

        bool operator==(const RenderHandle &other) const = default;
    };

    using LightHandle = RenderHandle<struct LightHandleTag>;
    // MeshHandle / TextureHandle / MaterialHandle land with their own migration sub-phase -
    // added when there is something behind them, not up front.
} // namespace BreadEngine
