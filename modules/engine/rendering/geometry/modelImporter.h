#pragma once
#include <string>
#include <vector>

#include "meshData.h"

namespace BreadEngine {
    /// One drawable part of an imported model: its geometry, and the index into the model's
    /// material list that it is drawn with.
    struct ModelPart
    {
        MeshData geometry;
        int materialSlot = 0;
    };

    /**
     * A model file as the engine keeps it. The source file's node hierarchy is flattened: each
     * part's vertices already carry the transform of the node it was found under, so a part can
     * be drawn with nothing but the renderer's own object transform.
     */
    struct ModelData
    {
        std::vector<ModelPart> parts;

        [[nodiscard]] bool isEmpty() const { return parts.empty(); }
    };

    /**
     * Reads a model file into engine geometry. Returns empty data if the file cannot be read or
     * holds nothing drawable; the caller has to handle that.
     */
    [[nodiscard]] ModelData importModel(const std::string &path);
} // namespace BreadEngine
