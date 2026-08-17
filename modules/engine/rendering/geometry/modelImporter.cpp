#include "modelImporter.h"

#include <span>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "logger.h"

namespace BreadEngine {
    namespace {
        /**
         * Triangulation and a tangent basis are what the vertex layout requires; the generators
         * are what the file lacks. Handedness and winding are left alone - assimp keeps the
         * file's own right-handed, counter-clockwise convention, which is already the engine's.
         *
         * FlipUVs is not optional. assimp's internal UV origin is the bottom-left, so its glTF
         * importer flips V on the way in ("Flip Y coords" in glTF2Importer.cpp); the engine
         * samples with V down from the top-left, as glTF itself stores it. Without this the
         * flip stands and every imported texture is mirrored vertically against its unwrap.
         */
        constexpr unsigned int IMPORT_FLAGS = aiProcess_Triangulate
                                             | aiProcess_GenSmoothNormals
                                             | aiProcess_GenUVCoords
                                             | aiProcess_FlipUVs
                                             | aiProcess_CalcTangentSpace
                                             | aiProcess_JoinIdenticalVertices
                                             | aiProcess_ImproveCacheLocality;

        [[nodiscard]] Vector3 toVector3(const aiVector3D &vector)
        {
            return Vector3{vector.x, vector.y, vector.z};
        }

        /**
         * Copies one mesh out of the scene, with @p transform applied so the result no longer
         * depends on where in the hierarchy it was found. Directions take @p normalTransform
         * instead, which is @p transform's inverse transpose: under a non-uniform scale that is
         * what keeps a normal perpendicular to the surface it came from.
         */
        [[nodiscard]] MeshData toMeshData(const aiMesh &source, const aiMatrix4x4 &transform, const aiMatrix3x3 &normalTransform)
        {
            const auto direction = aiMatrix3x3{transform};

            MeshData mesh;
            mesh.vertices.reserve(source.mNumVertices);
            for (unsigned int vertex = 0; vertex < source.mNumVertices; ++vertex)
            {
                // The layout has no second UV set and no vertex colour, so a mesh that carries
                // them loses them here rather than in the shader.
                const auto uv = source.HasTextureCoords(0) ? source.mTextureCoords[0][vertex] : aiVector3D{};
                auto normal = normalTransform * source.mNormals[vertex];
                auto tangent = direction * (source.HasTangentsAndBitangents() ? source.mTangents[vertex] : aiVector3D{1, 0, 0});

                mesh.vertices.push_back(MeshVertex{
                    .position = toVector3(transform * source.mVertices[vertex]),
                    .normal = toVector3(normal.NormalizeSafe()),
                    .uv = {uv.x, uv.y},
                    .tangent = toVector3(tangent.NormalizeSafe())
                });
            }

            mesh.indices.reserve(source.mNumFaces * 3);
            for (unsigned int face = 0; face < source.mNumFaces; ++face)
            {
                // Triangulation leaves points and lines alone, and they have no place in a
                // triangle list.
                const auto &indices = source.mFaces[face];
                if (indices.mNumIndices != 3) continue;

                for (unsigned int corner = 0; corner < 3; ++corner) mesh.indices.push_back(indices.mIndices[corner]);
            }

            return mesh;
        }

        /// The first of @p types the material names a texture for, empty if it names none.
        [[nodiscard]] std::string firstTexturePath(const aiMaterial &material, const std::span<const aiTextureType> types)
        {
            for (const auto type: types)
            {
                aiString path;
                if (material.GetTexture(type, 0, &path) != AI_SUCCESS || path.length == 0) continue;

                // An embedded texture is reported as *<index> into the scene's own image list.
                // The asset pipeline resolves files in the project, so there is nothing to name.
                if (path.data[0] == '*') continue;

                return std::string{path.C_Str(), path.length};
            }

            return {};
        }

        /**
         * Reads the texture set a material declares. Each engine slot has more than one possible
         * source because the formats disagree: the metallic-roughness types a glTF sets come
         * first, then the classic ones older formats use for the same map.
         */
        [[nodiscard]] ModelMaterial toModelMaterial(const aiMaterial &material)
        {
            constexpr aiTextureType ALBEDO[]{aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE};
            constexpr aiTextureType NORMAL[]{aiTextureType_NORMALS};
            // glTF's own packed metalness-roughness image first. Failing that, any single map
            // covering part of the trio is still the best stand-in for the packed slot - and
            // occlusion last, since a file that has only that one usually shares the image.
            constexpr aiTextureType ORM[]{
                aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_METALNESS,
                aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_LIGHTMAP
            };
            constexpr aiTextureType EMISSION[]{aiTextureType_EMISSIVE};

            return ModelMaterial{
                .albedo = firstTexturePath(material, ALBEDO),
                .normal = firstTexturePath(material, NORMAL),
                .orm = firstTexturePath(material, ORM),
                .emission = firstTexturePath(material, EMISSION)
            };
        }

        /// Walks the hierarchy, accumulating each node's transform into the meshes beneath it.
        void appendNode(ModelData &model, const aiScene &scene, const aiNode &node, const aiMatrix4x4 &parentTransform)
        {
            const aiMatrix4x4 transform = parentTransform * node.mTransformation;
            auto normalTransform = aiMatrix3x3{transform};
            normalTransform.Inverse().Transpose();

            for (unsigned int index = 0; index < node.mNumMeshes; ++index)
            {
                const auto *source = scene.mMeshes[node.mMeshes[index]];
                if (source == nullptr || source->mNumVertices == 0 || !source->HasNormals()) continue;

                MeshData geometry = toMeshData(*source, transform, normalTransform);
                if (geometry.isEmpty()) continue;

                model.parts.push_back(ModelPart{
                    .geometry = std::move(geometry),
                    .materialSlot = static_cast<int>(source->mMaterialIndex)
                });
            }

            for (unsigned int child = 0; child < node.mNumChildren; ++child)
            {
                appendNode(model, scene, *node.mChildren[child], transform);
            }
        }
    } // namespace

    ModelData importModel(const std::string &path)
    {
        // The importer owns the scene it returns, so everything is copied out before it leaves.
        Assimp::Importer importer;
        const auto *scene = importer.ReadFile(path, IMPORT_FLAGS);
        if (scene == nullptr || scene->mRootNode == nullptr)
        {
            Logger::LogError("Failed to import model " + path + ": " + importer.GetErrorString());
            return {};
        }

        ModelData model;
        model.materials.reserve(scene->mNumMaterials);
        for (unsigned int index = 0; index < scene->mNumMaterials; ++index)
        {
            model.materials.push_back(toModelMaterial(*scene->mMaterials[index]));
        }

        appendNode(model, *scene, *scene->mRootNode, aiMatrix4x4{});

        if (model.isEmpty()) Logger::LogWarning("Model " + path + " holds no drawable geometry");

        return model;
    }
} // namespace BreadEngine
