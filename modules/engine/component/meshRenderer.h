#pragma once
#include "r3d_model.h"
#include "configs/meshAsset.h"
#include "core/component.h"

namespace BreadEngine {
    struct MeshRenderer : Component
    {
        MeshRenderer() = default;

        explicit MeshRenderer(Node *owner);

        ~MeshRenderer() override = default;

        void Load(const std::string &path);

    private:
        R3D_Model _nativeMeshRenderer = {};
        MeshAsset* _meshAsset = nullptr;

        INSPECTOR_BEGIN(MeshRenderer)
            INSPECT_FIELD(_meshAsset)
        INSPECTOR_END()
    };
} // BreadEngine
