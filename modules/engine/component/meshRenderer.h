#pragma once
#include <vector>

#include "data/primitives/meshPrimitiveData.h"
#include "configs/assets/meshAsset.h"
#include "core/component.h"
#include "data/material.h"
#include "rendering/renderHandles.h"

namespace BreadEngine {
    struct MeshRenderer : Component
    {
        MeshRenderer() = default;

        explicit MeshRenderer(Node *owner);

        ~MeshRenderer() override = default;

        void onCreate() override;

        /// Builds whichever of the two sources this renderer has: a generated primitive or
        /// an imported model.
        void load();

        void unload();

        [[nodiscard]] bool isLoaded() const;

        void setMeshAsset(MeshAsset *meshAsset);

        void setGeneratedMesh(MeshPrimitiveData &primitiveData);

    private:
        friend class MeshRendererSystem;
        /// A renderer either draws a generated primitive or an imported model, never both.
        MeshHandle _mesh{};
        ModelHandle _model{};
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
