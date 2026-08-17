#include "meshRenderer.h"

#include <algorithm>
#include <sstream>

#include "node.h"
#include "transform.h"
#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"
#include "rendering/renderer.h"
#include "rendering/geometry/primitiveGenerator.h"

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

        _parts = _meshAsset->acquire();
        if (_parts.empty()) return;

        _acquiredAsset = _meshAsset;
    }

    void MeshRenderer::unload()
    {
        _loadAttempted = false;

        if (_acquiredAsset != nullptr)
        {
            _acquiredAsset->release();
            _acquiredAsset = nullptr;
        }
        else if (!_parts.empty())
        {
            auto &renderer = Renderer::get();
            for (const auto &[mesh, materialSlot]: _parts) renderer.destroyMesh(mesh);
        }

        _parts.clear();
    }

    void MeshRenderer::onDestroy()
    {
        unload();
    }

    bool MeshRenderer::isLoaded() const
    {
        return !_parts.empty();
    }

    std::vector<MaterialLink> &MeshRenderer::getMaterials()
    {
        if (!isLoaded()) return _materials;

        size_t slots = 1;
        for (const auto &[mesh, materialSlot]: _parts) slots = std::max(slots, static_cast<size_t>(materialSlot) + 1);
        if (_materials.size() < slots) _materials.resize(slots);

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

        _meshAsset = nullptr;
        _meshPrimitiveData = serializeMeshData(primitiveData);
        createPrimitivePart(primitiveData);
        _loadAttempted = true;
    }

    void MeshRenderer::createPrimitivePart(const MeshPrimitiveData &primitiveData)
    {
        const auto forward = _owner->get<Transform>().getForward();
        const auto mesh = Renderer::get().createMesh(generatePrimitive(primitiveData, forward));
        if (mesh.isValid()) _parts.push_back(MeshPart{.mesh = mesh});
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
            createPrimitivePart(primitiveData);
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
