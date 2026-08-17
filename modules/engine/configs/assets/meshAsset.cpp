#include "meshAsset.h"

#include "rendering/renderer.h"
#include "rendering/geometry/modelImporter.h"

namespace BreadEngine {
    const std::vector<MeshPart> &MeshAsset::acquire()
    {
        if (_parts.empty()) importParts();
        if (!_parts.empty()) ++_references;

        return _parts;
    }

    void MeshAsset::release()
    {
        if (_references == 0) return;
        if (--_references > 0) return;

        auto &renderer = Renderer::get();
        for (const auto &[mesh, materialSlot]: _parts) renderer.destroyMesh(mesh);

        _parts.clear();
    }

    void MeshAsset::importParts()
    {
        const ModelData model = importModel(getAssetPath());
        auto &renderer = Renderer::get();

        _parts.reserve(model.parts.size());
        for (const auto &[geometry, materialSlot]: model.parts)
        {
            const auto mesh = renderer.createMesh(geometry);
            if (mesh.isValid()) _parts.push_back(MeshPart{.mesh = mesh, .materialSlot = materialSlot});
        }
    }
} // BreadEngine
