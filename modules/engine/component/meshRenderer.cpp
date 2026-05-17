#include "meshRenderer.h"

#include <iostream>

#include "node.h"
#include "transform.h"
#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/freePolyPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/slopePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"
#include "data/primitives/torusPrimitiveData.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MeshRenderer)
    REGISTER_COMPONENT(MeshRenderer)

    MeshRenderer::MeshRenderer(Node *owner)
    {
        _owner = owner;
    }

    void MeshRenderer::onCreate()
    {
        if (_meshPrimitiveData.empty()) return;
        deserializeMeshData(_meshPrimitiveData);
        _isLoaded = true;
    }

    void MeshRenderer::loadModel()
    {
        if (_meshAsset == nullptr)
        {
            _isLoaded = true;
            return;
        }

        const auto path = _meshAsset->getAssetPath().c_str();
        _nativeMeshRenderer = R3D_LoadModelEx(path, 0);
        _materials = _meshAsset->getMaterials();
        _isLoaded = _nativeMeshRenderer.meshes != nullptr;
        isChangedFromEditor = false;
    }

    void MeshRenderer::unload()
    {
        if (!_isLoaded) return;
        if (R3D_IsMeshValid(_nativeMesh))
        {
            R3D_UnloadMesh(_nativeMesh);
        }
        else
        {
            R3D_UnloadModel(_nativeMeshRenderer, false);
            for (auto &material: _materials)
            {
                material.unload();
            }
        }

        _isLoaded = false;
    }

    bool MeshRenderer::isLoaded() const
    {
        return _isLoaded && (_meshAsset != nullptr || R3D_IsMeshValid(_nativeMesh));
    }

    void MeshRenderer::setMeshAsset(MeshAsset *meshAsset)
    {
        _meshAsset = meshAsset;
        if (_isLoaded)
        {
            unload();
            loadModel();
        }
    }

    void MeshRenderer::setGeneratedMesh(const R3D_Mesh &mesh, MeshPrimitiveData &primitiveData)
    {
        unload();
        for (auto &material: _materials)
        {
            material.unload();
        }

        std::ostringstream stream;
        stream << primitiveData.serialize();
        _meshPrimitiveData = serializeMeshData(primitiveData);
        _nativeMesh = mesh;
        _materials = {Material()};
        _isLoaded = true;
    }

    std::string MeshRenderer::serializeMeshData(MeshPrimitiveData &primitiveData)
    {
        std::ostringstream stream;
        auto node = primitiveData.serialize();
        node["type"] = static_cast<int>(primitiveData.getMeshType());
        stream << node;
        return stream.str();
    }

    void MeshRenderer::deserializeMeshData(const std::string &data)
    {
        switch (auto rawNode = YAML::Load(data); static_cast<MeshPrimitiveType>(rawNode["type"].as<int>()))
        {
            case MeshPrimitiveType::Cube:
            {
                auto convertedData = CubePrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshCube(convertedData.width, convertedData.height, convertedData.depth);
                break;
            }
            case MeshPrimitiveType::Sphere:
            {
                auto convertedData = SpherePrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshSphere(convertedData.radius, convertedData.rings, convertedData.slices);
                break;
            }
            case MeshPrimitiveType::HalfSphere:
            {
                auto &convertedData = SpherePrimitiveData().asHalf();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshHemiSphere(convertedData.radius, convertedData.rings, convertedData.slices);
                break;
            }
            case MeshPrimitiveType::Cylinder:
            {
                auto convertedData = CylinderPrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshCylinderEx(convertedData.bottomRadius, convertedData.topRadius, convertedData.height, convertedData.slices, convertedData.stacks, convertedData.bottomCap, convertedData.topCap);
                break;
            }
            case MeshPrimitiveType::Capsule:
            {
                auto convertedData = CapsulePrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshCapsule(convertedData.radius, convertedData.height, convertedData.rings, convertedData.slices);
                break;
            }
            case MeshPrimitiveType::Plane:
            {
                auto convertedData = PlanePrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshPlane(convertedData.width, convertedData.height, convertedData.resX, convertedData.resZ);
                break;
            }
            case MeshPrimitiveType::Quad:
            {
                auto &convertedData = PlanePrimitiveData().asQuad();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshQuad(convertedData.width, convertedData.height, convertedData.resX, convertedData.resZ, _owner->get<Transform>().getForward());
                break;
            }
            case MeshPrimitiveType::Slope:
            {
                auto convertedData = SlopePrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshSlope(convertedData.width, convertedData.height, convertedData.length, convertedData.normal);
                break;
            }
            case MeshPrimitiveType::Torus:
            {
                auto convertedData = TorusPrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshTorus(convertedData.radius, convertedData.size, convertedData.radiusSegments, convertedData.sides);
                break;
            }
            case MeshPrimitiveType::FreePoly:
            {
                auto convertedData = FreePolyPrimitiveData();
                convertedData.deserialize(rawNode);
                _nativeMesh = R3D_GenMeshPoly(convertedData.sides, convertedData.size, _owner->get<Transform>().getForward());
                break;
            }
            case MeshPrimitiveType::None:
            default: break;
        }
    }
} // BreadEngine
