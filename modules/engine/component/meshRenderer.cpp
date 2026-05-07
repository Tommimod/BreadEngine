#include "meshRenderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MeshRenderer)
    REGISTER_COMPONENT(MeshRenderer)

    MeshRenderer::MeshRenderer(Node *owner)
    {
        _owner = owner;
    }

    void MeshRenderer::loadMesh()
    {
        if (_meshAsset == nullptr)
        {
            return;
        }

        const auto path = _meshAsset->getAssetPath().c_str();
        _nativeMeshRenderer = R3D_LoadModelEx(path, 0);
        _materials = _meshAsset->getMaterials();
        _isLoaded = _nativeMeshRenderer.meshes != nullptr;
    }

    void MeshRenderer::unload()
    {
        if (!_isLoaded) return;
        R3D_UnloadModel(_nativeMeshRenderer, false);
        for (auto &material: _materials)
        {
            material.unload();
        }

        _isLoaded = false;
    }

    bool MeshRenderer::isLoaded() const
    {
        return _isLoaded && _meshAsset != nullptr;
    }

    void MeshRenderer::setMeshAsset(MeshAsset *meshAsset)
    {
        _meshAsset = meshAsset;
        if (_isLoaded)
        {
            unload();
            loadMesh();
        }
    }
} // BreadEngine
