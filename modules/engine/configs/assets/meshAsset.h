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

        explicit MeshAsset(const std::string &fileGuid) : Asset(fileGuid)
        {
        }

        ~MeshAsset() override = default;

        [[nodiscard]] std::vector<Material> const &getMaterials() const { return _materials; }

        void loadToMemory() override;

    private:
        std::vector<Material> _materials;
        bool _isLoaded = false;

        INSPECTOR_BEGIN(MeshAsset)
            INSPECT_FIELD(_materials);
        INSPECTOR_END()
    };
} // BreadEngine
