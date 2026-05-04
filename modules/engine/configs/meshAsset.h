#pragma once
#include "asset.h"

namespace BreadEngine {
    struct MeshAsset : Asset
    {
        MeshAsset() : Asset()
        {
        }

        explicit MeshAsset(File *file) : Asset(file)
        {
        }

        ~MeshAsset() override = default;

    private:
        INSPECTOR_BEGIN(MeshAsset)
        INSPECTOR_END()
    };
} // BreadEngine
