#pragma once
#include <cstdint>

namespace BreadEngine {
    /**
     * Reference to a resource owned by the renderer.
     *
     * `generation` is bumped when a slot is freed, so a handle kept past a destroy resolves
     * to nothing instead of addressing whatever resource lands in that slot next. The Tag
     * parameter keeps the handle families from converting into each other.
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
    using TextureHandle = RenderHandle<struct TextureHandleTag>;
    using MaterialHandle = RenderHandle<struct MaterialHandleTag>;
    using MeshHandle = RenderHandle<struct MeshHandleTag>;
    using CubemapHandle = RenderHandle<struct CubemapHandleTag>;
    using AmbientMapHandle = RenderHandle<struct AmbientMapHandleTag>;
    using OverlayEffectHandle = RenderHandle<struct OverlayEffectHandleTag>;
    using OverlayMeshHandle = RenderHandle<struct OverlayMeshHandleTag>;
} // namespace BreadEngine
