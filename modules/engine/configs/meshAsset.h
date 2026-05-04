#pragma once
#include "asset.h"

namespace BreadEngine {
    struct MeshAsset : Asset
    {
        MeshAsset() : Asset()
        {
        }

        MeshAsset(const std::string &guid, const std::string &name) : Asset(guid, name)
        {
        }

        ~MeshAsset() override = default;

    private:
        INSPECTOR_BEGIN(MeshAsset)
        INSPECTOR_END()
    };
} // BreadEngine
