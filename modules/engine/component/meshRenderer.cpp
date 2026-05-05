#include "meshRenderer.h"

namespace BreadEngine {
    MeshRenderer::MeshRenderer(Node *owner)
    {
        _owner = owner;
    }

    void MeshRenderer::load()
    {
        if (_meshAsset == nullptr)
        {
            Logger::LogError("Can't load meshes, because asset link is null");
            return;
        }

        _nativeMeshRenderer = R3D_LoadModelEx(_meshAsset->getAssetPath().c_str(), R3D_IMPORT_MESH_DATA | R3D_IMPORT_QUALITY);
        _materials = _meshAsset->getMaterials();
        _isLoaded = true;
    }

    void MeshRenderer::unload()
    {
        if (!_isLoaded) return;
        R3D_UnloadModel(_nativeMeshRenderer, true);
        for (auto &material: _materials)
        {
            material.unload();
        }

        _isLoaded = false;
    }

    void MeshRenderer::setMeshAsset(MeshAsset *meshAsset)
    {
        _meshAsset = meshAsset;
        if (_isLoaded)
        {
            unload();
            load();
        }
    }

    DEFINE_STATIC_PROPS(MeshRenderer)
    REGISTER_COMPONENT(MeshRenderer)
} // BreadEngine
