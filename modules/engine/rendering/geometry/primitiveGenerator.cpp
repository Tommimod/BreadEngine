#include "primitiveGenerator.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

#include "raymath.h"

#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"

namespace BreadEngine {
    namespace {
        constexpr float FULL_TURN = 2.0f * PI;
        constexpr float QUARTER_TURN = 0.5f * PI;

        /**
         * Two triangles over one cell of a (u, v) grid, wound counter-clockwise as seen from
         * the front face. Corners are named in (u, v) order: @p a at (u0, v0), then around the
         * cell to @p d at (u0, v1).
         */
        void appendCell(MeshData &mesh, const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d)
        {
            for (const auto index: {d, c, b, d, b, a}) mesh.indices.push_back(index);
        }

        /**
         * Appends a flat rectangle centred on @p center facing @p normal, spanned by @p tangent
         * along increasing u and by cross(tangent, normal) - the direction v increases in -
         * along increasing v. Subdivided into @p resU by @p resV cells, with the UVs spanning
         * the whole rectangle.
         */
        void appendGrid(MeshData &mesh, const Vector3 center, const Vector3 normal, const Vector3 tangent,
                        const float extentU, const float extentV, const int resU, const int resV)
        {
            const Vector3 bitangent = Vector3CrossProduct(tangent, normal);
            const auto base = static_cast<uint32_t>(mesh.vertices.size());
            const auto columns = static_cast<uint32_t>(resU) + 1;

            for (int row = 0; row <= resV; ++row)
            {
                const float v = static_cast<float>(row) / static_cast<float>(resV);
                for (int column = 0; column <= resU; ++column)
                {
                    const float u = static_cast<float>(column) / static_cast<float>(resU);
                    const Vector3 alongU = Vector3Scale(tangent, (u - 0.5f) * extentU);
                    const Vector3 alongV = Vector3Scale(bitangent, (v - 0.5f) * extentV);

                    mesh.vertices.push_back(MeshVertex{
                        .position = Vector3Add(center, Vector3Add(alongU, alongV)),
                        .normal = normal,
                        .uv = {u, v},
                        .tangent = tangent
                    });
                }
            }

            for (uint32_t row = 0; row < static_cast<uint32_t>(resV); ++row)
            {
                for (uint32_t column = 0; column < static_cast<uint32_t>(resU); ++column)
                {
                    const auto corner = base + row * columns + column;
                    appendCell(mesh, corner, corner + 1, corner + 1 + columns, corner + columns);
                }
            }
        }

        /// Reach of a box of @p size along one of its own axes.
        float extentAlong(const Vector3 axis, const Vector3 size)
        {
            return std::fabs(Vector3DotProduct(axis, size));
        }

        /// Six independent faces, so each keeps its own normal and its own texture span.
        MeshData buildBox(const Vector3 size)
        {
            struct BoxFace
            {
                Vector3 normal;
                /// Along increasing u, chosen per face so the texture is upright from outside.
                Vector3 tangent;
            };

            constexpr BoxFace FACES[]{
                {{1, 0, 0}, {0, 0, -1}},
                {{-1, 0, 0}, {0, 0, 1}},
                {{0, 1, 0}, {1, 0, 0}},
                {{0, -1, 0}, {1, 0, 0}},
                {{0, 0, 1}, {1, 0, 0}},
                {{0, 0, -1}, {-1, 0, 0}}
            };

            MeshData mesh;
            mesh.vertices.reserve(4 * std::size(FACES));
            mesh.indices.reserve(6 * std::size(FACES));

            for (const auto &[normal, tangent]: FACES)
            {
                const Vector3 center = Vector3Scale(normal, 0.5f * extentAlong(normal, size));
                const Vector3 bitangent = Vector3CrossProduct(tangent, normal);
                appendGrid(mesh, center, normal, tangent, extentAlong(tangent, size), extentAlong(bitangent, size), 1, 1);
            }

            return mesh;
        }

        /**
         * One ring of a surface of revolution around the Y axis: the circle it spans, the
         * normal every vertex on it points along, and the v it samples the texture at.
         */
        struct RevolutionRing
        {
            float y = 0;
            float radius = 0;
            /// Normal split into its radial and axial parts, already normalised together: the
            /// vertex at angle theta takes normal (radial * sin(theta), axial, radial * cos(theta)).
            float normalRadial = 1;
            float normalAxial = 0;
            float v = 0;
        };

        /**
         * Appends the surface passing through @p rings in order, closed around the Y axis in
         * @p slices steps. The rings must run in the direction v increases. The seam is
         * duplicated rather than shared, so the texture wraps across it instead of being
         * compressed back to u = 0.
         */
        void appendRevolution(MeshData &mesh, const std::span<const RevolutionRing> rings, const int slices)
        {
            const auto base = static_cast<uint32_t>(mesh.vertices.size());
            const auto columns = static_cast<uint32_t>(slices) + 1;
            mesh.vertices.reserve(mesh.vertices.size() + rings.size() * columns);

            for (const auto &ring: rings)
            {
                for (int slice = 0; slice <= slices; ++slice)
                {
                    const float u = static_cast<float>(slice) / static_cast<float>(slices);
                    const float theta = u * FULL_TURN;
                    const float sinTheta = std::sin(theta);
                    const float cosTheta = std::cos(theta);

                    mesh.vertices.push_back(MeshVertex{
                        .position = {ring.radius * sinTheta, ring.y, ring.radius * cosTheta},
                        .normal = {ring.normalRadial * sinTheta, ring.normalAxial, ring.normalRadial * cosTheta},
                        .uv = {u, ring.v},
                        // Along increasing u, which makes cross(tangent, normal) the direction v
                        // increases in for every ring this builds.
                        .tangent = {cosTheta, 0, -sinTheta}
                    });
                }
            }

            for (uint32_t row = 0; row + 1 < rings.size(); ++row)
            {
                for (uint32_t slice = 0; slice < static_cast<uint32_t>(slices); ++slice)
                {
                    const auto corner = base + row * columns + slice;
                    appendCell(mesh, corner, corner + 1, corner + 1 + columns, corner + columns);
                }
            }
        }

        /**
         * Appends the flat disc of @p radius at height @p y that closes a surface of
         * revolution, facing up or down per @p facingUp. The texture is projected straight
         * down the axis, matching the box's own top and bottom faces rather than continuing
         * the cylindrical mapping of the wall it closes.
         */
        void appendCap(MeshData &mesh, const float y, const float radius, const int slices, const bool facingUp)
        {
            const float axial = facingUp ? 1.0f : -1.0f;
            const Vector3 normal{0, axial, 0};
            constexpr Vector3 tangent{1, 0, 0};

            const auto center = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back(MeshVertex{.position = {0, y, 0}, .normal = normal, .uv = {0.5f, 0.5f}, .tangent = tangent});

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float theta = static_cast<float>(slice) / static_cast<float>(slices) * FULL_TURN;
                const float x = radius * std::sin(theta);
                const float z = radius * std::cos(theta);

                mesh.vertices.push_back(MeshVertex{
                    .position = {x, y, z},
                    .normal = normal,
                    // cross(tangent, normal) is +Z facing up and -Z facing down, so v follows z
                    // through the same sign flip.
                    .uv = {0.5f + 0.5f * x / radius, 0.5f + 0.5f * axial * z / radius},
                    .tangent = tangent
                });
            }

            for (uint32_t slice = 0; slice < static_cast<uint32_t>(slices); ++slice)
            {
                // Increasing theta runs from +Z towards +X, which is counter-clockwise seen
                // from above - so only the downward-facing disc has to be reversed.
                const auto edge = center + 1 + slice;
                mesh.indices.push_back(center);
                mesh.indices.push_back(facingUp ? edge : edge + 1);
                mesh.indices.push_back(facingUp ? edge + 1 : edge);
            }
        }

        /**
         * Latitude bands from the north pole down, so v runs the way the texture's own v does.
         * @p polarSweep is how much of the pole-to-pole turn the surface covers: all of it for
         * a sphere, a quarter turn for a dome that stops at the equator.
         *
         * The poles are ordinary rings of zero radius rather than single vertices, which gives
         * each one its own u and keeps the texture from pinching; the cells against them
         * degenerate into triangles.
         */
        MeshData buildSphere(const float radius, const int rings, const int slices, const float polarSweep)
        {
            std::vector<RevolutionRing> latitudes(rings + 1);
            for (int ring = 0; ring <= rings; ++ring)
            {
                const float v = static_cast<float>(ring) / static_cast<float>(rings);
                const float polar = v * polarSweep;
                latitudes[ring] = RevolutionRing{
                    .y = radius * std::cos(polar),
                    .radius = radius * std::sin(polar),
                    .normalRadial = std::sin(polar),
                    .normalAxial = std::cos(polar),
                    .v = v
                };
            }

            MeshData mesh;
            appendRevolution(mesh, latitudes, slices);
            return mesh;
        }

        /// A cylinder, a cone or a truncated cone, centred on the origin and extending
        /// half of @p height either side of it.
        MeshData buildCylinder(const float bottomRadius, const float topRadius, const float height,
                               const int slices, const int stacks, const bool bottomCap, const bool topCap)
        {
            // The wall's normal tilts with the slope between the two radii: purely radial for a
            // straight cylinder, leaning towards the narrow end for a cone.
            const Vector3 slopeNormal = Vector3Normalize(Vector3{height, bottomRadius - topRadius, 0});

            std::vector<RevolutionRing> wall(stacks + 1);
            for (int stack = 0; stack <= stacks; ++stack)
            {
                const float v = static_cast<float>(stack) / static_cast<float>(stacks);
                // v runs downward, so the first ring is the top one.
                wall[stack] = RevolutionRing{
                    .y = (0.5f - v) * height,
                    .radius = Lerp(topRadius, bottomRadius, v),
                    .normalRadial = slopeNormal.x,
                    .normalAxial = slopeNormal.y,
                    .v = v
                };
            }

            MeshData mesh;
            appendRevolution(mesh, wall, slices);
            // A cone's tip has no disc to close, and asking for one would divide by its radius.
            if (topCap && topRadius > 0) appendCap(mesh, 0.5f * height, topRadius, slices, true);
            if (bottomCap && bottomRadius > 0) appendCap(mesh, -0.5f * height, bottomRadius, slices, false);

            return mesh;
        }

        /**
         * A straight section of @p height capped with two hemispheres of @p radius, so the
         * whole shape reaches height + 2 * radius. The serialized height is the straight
         * section alone - that reading is what makes the stored defaults (height and radius
         * both 1) a capsule rather than a shape with negative length.
         */
        MeshData buildCapsule(const float radius, const float height, const int rings, const int slices)
        {
            // v is handed out in proportion to arc length, so the texture crosses the caps and
            // the straight section at one rate.
            const float capArc = QUARTER_TURN * radius;
            const float totalArc = height + 2.0f * capArc;
            const float halfHeight = 0.5f * height;

            std::vector<RevolutionRing> surface;
            surface.reserve(2 * (rings + 1));
            const auto append = [&](const float polar, const float axisOffset, const float arc)
            {
                surface.push_back(RevolutionRing{
                    .y = axisOffset + radius * std::cos(polar),
                    .radius = radius * std::sin(polar),
                    .normalRadial = std::sin(polar),
                    .normalAxial = std::cos(polar),
                    // A capsule with no radius and no height has no arc to divide by, and the
                    // inspector can ask for one. Collapsed geometry is a better answer than
                    // NaN positions, which reach the GPU as garbage rather than as nothing.
                    .v = totalArc > 0 ? arc / totalArc : 0.0f
                });
            };

            for (int ring = 0; ring <= rings; ++ring)
            {
                const float polar = static_cast<float>(ring) / static_cast<float>(rings) * QUARTER_TURN;
                append(polar, halfHeight, polar * radius);
            }

            // The equator appears twice, once at each end of the straight section: those two
            // rings share a radius and a normal, so the cell between them is the wall.
            for (int ring = 0; ring <= rings; ++ring)
            {
                const float polar = QUARTER_TURN * (1.0f + static_cast<float>(ring) / static_cast<float>(rings));
                append(polar, -halfHeight, capArc + height + (polar - QUARTER_TURN) * radius);
            }

            MeshData mesh;
            appendRevolution(mesh, surface, slices);
            return mesh;
        }

        /**
         * The tangent of a rectangle facing @p normal, kept horizontal so the rectangle stays
         * upright. A normal pointing straight up or down leaves no horizon to take that tangent
         * from, so the reference direction swaps to one that does.
         */
        Vector3 gridTangentFor(const Vector3 normal)
        {
            constexpr float AXIS_ALIGNED = 0.999f;
            const Vector3 reference = std::fabs(normal.y) < AXIS_ALIGNED
                                          ? Vector3{0, 1, 0}
                                          : Vector3{0, 0, normal.y > 0 ? -1.0f : 1.0f};

            return Vector3Normalize(Vector3CrossProduct(reference, normal));
        }
    } // namespace

    MeshData generatePrimitive(const MeshPrimitiveData &data, const Vector3 forward)
    {
        // Subdivision counts come from the inspector unclamped, and every generator divides by
        // them; a surface of revolution also needs three slices before it encloses anything.
        const auto atLeast = [](const int value, const int minimum) { return std::max(value, minimum); };

        switch (data.getMeshType())
        {
            case MeshPrimitiveType::Cube:
            {
                const auto &cube = static_cast<const CubePrimitiveData &>(data);
                return buildBox(Vector3{cube.width, cube.height, cube.depth});
            }
            case MeshPrimitiveType::Sphere:
            {
                const auto &sphere = static_cast<const SpherePrimitiveData &>(data);
                return buildSphere(sphere.radius, atLeast(sphere.rings, 2), atLeast(sphere.slices, 3), PI);
            }
            case MeshPrimitiveType::HalfSphere:
            {
                const auto &sphere = static_cast<const SpherePrimitiveData &>(data);
                return buildSphere(sphere.radius, atLeast(sphere.rings, 1), atLeast(sphere.slices, 3), QUARTER_TURN);
            }
            case MeshPrimitiveType::Cylinder:
            {
                const auto &cylinder = static_cast<const CylinderPrimitiveData &>(data);
                return buildCylinder(cylinder.bottomRadius, cylinder.topRadius, cylinder.height,
                                     atLeast(cylinder.slices, 3), atLeast(cylinder.stacks, 1),
                                     cylinder.bottomCap, cylinder.topCap);
            }
            case MeshPrimitiveType::Capsule:
            {
                const auto &capsule = static_cast<const CapsulePrimitiveData &>(data);
                return buildCapsule(capsule.radius, capsule.height, atLeast(capsule.rings, 1), atLeast(capsule.slices, 3));
            }
            case MeshPrimitiveType::Plane:
            {
                const auto &plane = static_cast<const PlanePrimitiveData &>(data);
                MeshData mesh;
                appendGrid(mesh, Vector3{}, Vector3{0, 1, 0}, Vector3{1, 0, 0}, plane.width, plane.height,
                           atLeast(plane.resX, 1), atLeast(plane.resZ, 1));
                return mesh;
            }
            case MeshPrimitiveType::Quad:
            {
                const auto &quad = static_cast<const PlanePrimitiveData &>(data);
                const Vector3 normal = Vector3Normalize(forward);
                MeshData mesh;
                appendGrid(mesh, Vector3{}, normal, gridTangentFor(normal), quad.width, quad.height,
                           atLeast(quad.resX, 1), atLeast(quad.resZ, 1));
                return mesh;
            }
            case MeshPrimitiveType::None:
            default: return {};
        }
    }
} // namespace BreadEngine
