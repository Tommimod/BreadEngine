#pragma once
#include "meshData.h"

namespace BreadEngine {
    struct MeshPrimitiveData;

    /**
     * Builds the geometry a scene's primitive description asks for. Returns empty geometry
     * for a primitive type that has no generator, which the caller has to handle.
     *
     * @param forward orientation for primitives built around a facing direction (quad, poly).
     */
    [[nodiscard]] MeshData generatePrimitive(const MeshPrimitiveData &data, Vector3 forward);
} // namespace BreadEngine
