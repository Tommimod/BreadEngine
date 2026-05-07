#pragma once
#include <vector>

#include "r3d_model.h"
#include "configs/meshAsset.h"
#include "core/component.h"
#include "data/material.h"

namespace BreadEngine {
    struct MeshRenderer : Component
    {
        MeshRenderer() = default;

        explicit MeshRenderer(Node *owner);

        ~MeshRenderer() override = default;

        void loadMesh();

        void unload();

        [[nodiscard]] bool isLoaded() const;

        void setMeshAsset(MeshAsset *meshAsset);

    private:
        friend class MeshRendererSystem;
        R3D_Model _nativeMeshRenderer = {};
        std::vector<Material> _materials;
        MeshAsset *_meshAsset = nullptr;
        bool _isLoaded = false;

        INSPECTOR_BEGIN(MeshRenderer)
            INSPECT_FIELD(_meshAsset)
            INSPECT_FIELD(_materials)
        INSPECTOR_END()
    };
} // BreadEngine
