#pragma once
#include <cstdint>
#include <vector>

#include "raylib.h"

namespace BreadEngine {
    /**
     * One vertex in the layout every renderer buffer and every input layout is built from.
     * The attribute set mirrors DiligentFX's PBR_Renderer, whose pipelines these buffers are
     * meant to feed unchanged.
     */
    struct MeshVertex
    {
        Vector3 position{};
        Vector3 normal{};
        Vector2 uv{};
        /// Points along increasing u. The bitangent carries no handedness of its own and is
        /// derived as cross(normal, tangent), which is why this is a direction, not a plane.
        Vector3 tangent{};
    };

    /// Geometry as the CPU builds it, before a backend uploads it. Triangle list, with
    /// counter-clockwise winding on front faces.
    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;

        [[nodiscard]] bool isEmpty() const { return vertices.empty() || indices.empty(); }
    };
} // namespace BreadEngine
