#pragma once
#include <vector>

#include "data/primitives/meshPrimitiveData.h"
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

        void onCreate() override;

        void loadModel();

        void unload();

        [[nodiscard]] bool isLoaded() const;

        void setMeshAsset(MeshAsset *meshAsset);

        void setGeneratedMesh(const R3D_Mesh &mesh, MeshPrimitiveData &primitiveData);

    private:
        friend class MeshRendererSystem;
        R3D_Model _nativeMeshRenderer = {};
        R3D_Mesh _nativeMesh = {};
        std::string _meshPrimitiveData;
        std::vector<Material> _materials;
        MeshAsset *_meshAsset = nullptr;
        bool _isLoaded = false;

        static std::string serializeMeshData(MeshPrimitiveData &primitiveData);

        void deserializeMeshData(const std::string &data);

        INSPECTOR_BEGIN(MeshRenderer)
            INSPECT_FIELD_OPT(_meshPrimitiveData, Property::Options::HIDDEN)
            INSPECT_FIELD(_meshAsset)
            INSPECT_FIELD(_materials)
        INSPECTOR_END()
    };
} // BreadEngine
