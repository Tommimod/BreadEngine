#pragma once
#include <vector>

#include "data/primitives/meshPrimitiveData.h"
#include "configs/assets/meshAsset.h"
#include "core/component.h"
#include "data/material.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct MeshRenderer : Component
    {
        MeshRenderer() = default;

        explicit MeshRenderer(Node *owner);

        ~MeshRenderer() override = default;

        void onCreate() override;

        void onDestroy() override;

        /// Builds whichever of the two sources this renderer has: a generated primitive or
        /// an imported model.
        void load();

        void unload();

        [[nodiscard]] bool isLoaded() const;

        /**
         * The materials this renderer draws with, never empty while it has something to draw:
         * a mesh renders with the default material rather than not at all. The list can be
         * emptied from the inspector at any time and raises no flag doing so, so the guarantee
         * is restored here rather than only in load().
         */
        [[nodiscard]] std::vector<Material> &getMaterials();

        void setMeshAsset(MeshAsset *meshAsset);

        void setGeneratedMesh(MeshPrimitiveData &primitiveData);

    private:
        friend class MeshRendererSystem;
        std::vector<MeshPart> _parts;
        std::string _meshPrimitiveData;
        std::vector<Material> _materials;
        MeshAsset *_meshAsset = nullptr;
        /**
         * The asset the parts were acquired from. It is both what the reference has to go back
         * to and what says the parts are borrowed rather than owned - the inspector rewrites
         * _meshAsset behind this component's back, so that field cannot be trusted at unload.
         */
        MeshAsset *_acquiredAsset = nullptr;
        /// Whether load() has already run for the current source. Latches a failed load so a
        /// missing model is not re-imported from disk every frame; unload() clears it.
        bool _loadAttempted = false;

        static std::string serializeMeshData(MeshPrimitiveData &primitiveData);

        void deserializeMeshData(const std::string &data);

        /// Generates @p primitiveData's geometry and takes the single part it becomes.
        void createPrimitivePart(const MeshPrimitiveData &primitiveData);

        INSPECTOR_BEGIN(MeshRenderer)
            INSPECT_FIELD_OPT(_meshPrimitiveData, Property::Options::HIDDEN)
            INSPECT_FIELD(_meshAsset)
            INSPECT_FIELD(_materials)
        INSPECTOR_END()
    };
} // BreadEngine
