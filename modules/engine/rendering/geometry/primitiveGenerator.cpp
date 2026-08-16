#include "primitiveGenerator.h"

#include <cmath>

#include "raymath.h"

#include "data/primitives/cubePrimitiveData.h"

namespace BreadEngine {
    namespace {
        /// The four corners of a face, in (u, v) order. Index 0 is the corner at the texture's
        /// origin and they run around the face from there.
        constexpr Vector2 FACE_CORNERS[]{{0, 0}, {1, 0}, {1, 1}, {0, 1}};

        /// Wound counter-clockwise as seen from outside, given FACE_CORNERS' order.
        constexpr uint32_t FACE_INDICES[]{3, 2, 1, 3, 1, 0};

        /// Reach of a box of @p size along one of its own axes.
        float extentAlong(const Vector3 axis, const Vector3 size)
        {
            return std::fabs(Vector3DotProduct(axis, size));
        }

        /**
         * Appends the face of a box centred on the origin that @p normal points out of,
         * spanned by @p tangent along increasing u.
         */
        void appendBoxFace(MeshData &mesh, const Vector3 normal, const Vector3 tangent, const Vector3 size)
        {
            // Increasing v runs down the face as seen from outside, which is the opposite of
            // what cross(normal, tangent) gives - hence the operands this way round.
            const Vector3 bitangent = Vector3CrossProduct(tangent, normal);
            const Vector3 center = Vector3Scale(normal, 0.5f * extentAlong(normal, size));
            const float extentU = extentAlong(tangent, size);
            const float extentV = extentAlong(bitangent, size);

            const auto base = static_cast<uint32_t>(mesh.vertices.size());
            for (const auto corner: FACE_CORNERS)
            {
                const Vector3 alongU = Vector3Scale(tangent, (corner.x - 0.5f) * extentU);
                const Vector3 alongV = Vector3Scale(bitangent, (corner.y - 0.5f) * extentV);

                mesh.vertices.push_back(MeshVertex{
                    .position = Vector3Add(center, Vector3Add(alongU, alongV)),
                    .normal = normal,
                    .uv = corner,
                    .tangent = tangent
                });
            }

            for (const auto index: FACE_INDICES) mesh.indices.push_back(base + index);
        }

        /// Six independent faces, so each keeps its own normal and its own texture span.
        MeshData buildBox(const Vector3 size)
        {
            MeshData mesh;
            mesh.vertices.reserve(std::size(FACE_CORNERS) * 6);
            mesh.indices.reserve(std::size(FACE_INDICES) * 6);

            appendBoxFace(mesh, Vector3{1, 0, 0}, Vector3{0, 0, -1}, size);
            appendBoxFace(mesh, Vector3{-1, 0, 0}, Vector3{0, 0, 1}, size);
            appendBoxFace(mesh, Vector3{0, 1, 0}, Vector3{1, 0, 0}, size);
            appendBoxFace(mesh, Vector3{0, -1, 0}, Vector3{1, 0, 0}, size);
            appendBoxFace(mesh, Vector3{0, 0, 1}, Vector3{1, 0, 0}, size);
            appendBoxFace(mesh, Vector3{0, 0, -1}, Vector3{-1, 0, 0}, size);

            return mesh;
        }
    } // namespace

    MeshData generatePrimitive(const MeshPrimitiveData &data, const Vector3 forward)
    {
        switch (data.getMeshType())
        {
            case MeshPrimitiveType::Cube:
            {
                const auto &cube = static_cast<const CubePrimitiveData &>(data);
                return buildBox(Vector3{cube.width, cube.height, cube.depth});
            }
            default: return {};
        }
    }
} // namespace BreadEngine
