#pragma once
#include <vector>

#include "asset.h"
#include "data/material.h"

namespace BreadEngine {
    struct MeshAsset : Asset
    {
        MeshAsset() : Asset()
        {
        }

        explicit MeshAsset(File *file);

        ~MeshAsset() override = default;

        std::vector<Material> const &getMaterials() const { return _materials; }

    private:
        std::vector<Material> _materials;

        INSPECTOR_BEGIN(MeshAsset)
            INSPECT_FIELD(_materials);
        INSPECTOR_END()
    };
} // BreadEngine
