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
     * The textures one of the model's materials names, as paths relative to the model file.
     * A slot the file gives no texture for stays empty. Resolving these against the project is
     * the caller's job - the importer reads the file and touches no assets.
     */
    struct ModelMaterial
    {
        std::string albedo;
        std::string normal;
        std::string orm;
        std::string emission;
    };

    /**
     * A model file as the engine keeps it. The source file's node hierarchy is flattened: each
     * part's vertices already carry the transform of the node it was found under, so a part can
     * be drawn with nothing but the renderer's own object transform.
     */
    struct ModelData
    {
        std::vector<ModelPart> parts;
        std::vector<ModelMaterial> materials;

        [[nodiscard]] bool isEmpty() const { return parts.empty(); }
    };

    /**
     * Reads a model file into engine geometry. Returns empty data if the file cannot be read or
     * holds nothing drawable; the caller has to handle that.
     */
    [[nodiscard]] ModelData importModel(const std::string &path);
} // namespace BreadEngine
