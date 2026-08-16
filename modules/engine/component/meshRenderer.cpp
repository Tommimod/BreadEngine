#include "meshRenderer.h"

#include <sstream>

#include "node.h"
#include "transform.h"
#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"
#include "rendering/renderer.h"

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
        load();
    }

    void MeshRenderer::load()
    {
        if (_loadAttempted) return;

        _loadAttempted = true;
        if (_meshAsset == nullptr)
        {
            if (_meshPrimitiveData.empty()) return;

            deserializeMeshData(_meshPrimitiveData);
            return;
        }

        _model = Renderer::get().loadModel(_meshAsset->getAssetPath());
        _materials = _meshAsset->getMaterials();
    }

    void MeshRenderer::unload()
    {
        _loadAttempted = false;

        auto &renderer = Renderer::get();
        if (_mesh.isValid())
        {
            renderer.destroyMesh(_mesh);
            _mesh = {};
        }

        if (_model.isValid())
        {
            renderer.destroyModel(_model);
            _model = {};
        }
    }

    void MeshRenderer::onDestroy()
    {
        unload();
    }

    bool MeshRenderer::isLoaded() const
    {
        return _mesh.isValid() || _model.isValid();
    }

    std::vector<Material> &MeshRenderer::getMaterials()
    {
        if (isLoaded() && _materials.empty()) _materials.emplace_back();
        return _materials;
    }

    void MeshRenderer::setMeshAsset(MeshAsset *meshAsset)
    {
        unload();
        _meshAsset = meshAsset;
        load();
    }

    void MeshRenderer::setGeneratedMesh(MeshPrimitiveData &primitiveData)
    {
        unload();

        // A generated primitive replaces the imported model outright - holding both would
        // leave load() silently preferring the asset over the mesh just built here.
        _meshAsset = nullptr;
        _meshPrimitiveData = serializeMeshData(primitiveData);
        _mesh = Renderer::get().createPrimitive(primitiveData, _owner->get<Transform>().getForward());
        _materials = {Material()};
        _loadAttempted = true;
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
        const auto rawNode = YAML::Load(data);
        const auto build = [&](MeshPrimitiveData &primitiveData)
        {
            primitiveData.deserialize(rawNode);
            _mesh = Renderer::get().createPrimitive(primitiveData, _owner->get<Transform>().getForward());
        };

        switch (static_cast<MeshPrimitiveType>(rawNode["type"].as<int>()))
        {
            case MeshPrimitiveType::Cube:
            {
                CubePrimitiveData cube;
                build(cube);
                break;
            }
            case MeshPrimitiveType::Sphere:
            {
                SpherePrimitiveData sphere;
                build(sphere);
                break;
            }
            case MeshPrimitiveType::HalfSphere:
            {
                SpherePrimitiveData sphere;
                build(sphere.asHalf());
                break;
            }
            case MeshPrimitiveType::Cylinder:
            {
                CylinderPrimitiveData cylinder;
                build(cylinder);
                break;
            }
            case MeshPrimitiveType::Capsule:
            {
                CapsulePrimitiveData capsule;
                build(capsule);
                break;
            }
            case MeshPrimitiveType::Plane:
            {
                PlanePrimitiveData plane;
                build(plane);
                break;
            }
            case MeshPrimitiveType::Quad:
            {
                PlanePrimitiveData quad;
                build(quad.asQuad());
                break;
            }
            case MeshPrimitiveType::None:
            default: break;
        }
    }
} // BreadEngine
