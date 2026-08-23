#include "gizmoRenderer.h"

#include <algorithm>
#include <cmath>

#include "raymath.h"
#include "rendering/renderer.h"

namespace BreadEditor {
    using BreadEngine::OverlayMeshData;
    using BreadEngine::OverlayVertex;

    /// The colour of each axis handle, and of the free-move ring that belongs to none of them.
    /// In the space the output texture already holds: the overlay draws after the composite,
    /// so what is written here is what is shown.
    constexpr Vector4 AXIS_COLOR[3]{
        {0.898f, 0.282f, 0.357f, 1.0f},
        {0.514f, 0.804f, 0.220f, 1.0f},
        {0.271f, 0.541f, 0.949f, 1.0f}
    };
    constexpr Vector4 FREE_MOVE_COLOR{1.0f, 1.0f, 1.0f, 0.784f};

    namespace {
        constexpr int CONE_SEGMENTS = 16;
        constexpr int RING_SEGMENTS = 48;
        /// How much of a plane handle's own side its opaque border takes, leaving the rest to
        /// the translucent fill.
        constexpr float PLANE_BORDER = 0.08f;
        constexpr unsigned char PLANE_FILL_ALPHA = 128;

        /// Appends a triangle wound counter-clockwise as seen from the side it faces, which is
        /// the winding every pipeline in the tree treats as the front.
        void addTriangle(OverlayMeshData &mesh, const Vector3 &a, const Vector3 &b, const Vector3 &c,
                         const unsigned char alpha = 255)
        {
            const auto base = static_cast<uint32_t>(mesh.vertices.size());
            const Color tint{255, 255, 255, alpha};
            mesh.vertices.push_back(OverlayVertex{.position = a, .color = tint});
            mesh.vertices.push_back(OverlayVertex{.position = b, .color = tint});
            mesh.vertices.push_back(OverlayVertex{.position = c, .color = tint});
            mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2});
        }

        void addQuad(OverlayMeshData &mesh, const Vector3 &a, const Vector3 &b, const Vector3 &c, const Vector3 &d,
                     const unsigned char alpha = 255)
        {
            addTriangle(mesh, a, b, c, alpha);
            addTriangle(mesh, a, c, d, alpha);
        }

        /// Both windings of the same quad, for a face thin enough to be looked at from behind.
        /// Only one of the two survives the cull for any given eye, so the pair costs nothing
        /// beyond its vertices.
        void addDoubleSidedQuad(OverlayMeshData &mesh, const Vector3 &a, const Vector3 &b, const Vector3 &c,
                                const Vector3 &d, const unsigned char alpha = 255)
        {
            addQuad(mesh, a, b, c, d, alpha);
            addQuad(mesh, d, c, b, a, alpha);
        }

        /// A closed box spanning @p lower to @p upper, every face wound outward.
        void addBox(OverlayMeshData &mesh, const Vector3 &lower, const Vector3 &upper)
        {
            addQuad(mesh, {lower.x, lower.y, lower.z}, {lower.x, lower.y, upper.z},
                    {lower.x, upper.y, upper.z}, {lower.x, upper.y, lower.z});
            addQuad(mesh, {upper.x, lower.y, lower.z}, {upper.x, upper.y, lower.z},
                    {upper.x, upper.y, upper.z}, {upper.x, lower.y, upper.z});
            addQuad(mesh, {lower.x, lower.y, lower.z}, {upper.x, lower.y, lower.z},
                    {upper.x, lower.y, upper.z}, {lower.x, lower.y, upper.z});
            addQuad(mesh, {lower.x, upper.y, lower.z}, {lower.x, upper.y, upper.z},
                    {upper.x, upper.y, upper.z}, {upper.x, upper.y, lower.z});
            addQuad(mesh, {lower.x, lower.y, lower.z}, {lower.x, upper.y, lower.z},
                    {upper.x, upper.y, lower.z}, {upper.x, lower.y, lower.z});
            addQuad(mesh, {lower.x, lower.y, upper.z}, {upper.x, lower.y, upper.z},
                    {upper.x, upper.y, upper.z}, {lower.x, upper.y, upper.z});
        }

        [[nodiscard]] Vector3 onCircle(const int segment, const int segmentCount, const float radius, const float x)
        {
            const float angle = 2.0f * PI * static_cast<float>(segment) / static_cast<float>(segmentCount);
            return Vector3{x, std::cos(angle) * radius, std::sin(angle) * radius};
        }

        /// A square prism from the origin to +x, one unit across in each of the other two.
        [[nodiscard]] OverlayMeshData buildShaft()
        {
            OverlayMeshData mesh;
            addBox(mesh, {0.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
            return mesh;
        }

        [[nodiscard]] OverlayMeshData buildCube()
        {
            OverlayMeshData mesh;
            addBox(mesh, {-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
            return mesh;
        }

        /// A cone with its base circle of radius one at the origin and its apex at +x, capped
        /// so that it is closed from every direction the gizmo can be looked at from.
        [[nodiscard]] OverlayMeshData buildCone()
        {
            OverlayMeshData mesh;
            constexpr Vector3 apex{1.0f, 0.0f, 0.0f};
            constexpr Vector3 center{0.0f, 0.0f, 0.0f};
            for (int segment = 0; segment < CONE_SEGMENTS; ++segment)
            {
                const Vector3 current = onCircle(segment, CONE_SEGMENTS, 1.0f, 0.0f);
                const Vector3 next = onCircle(segment + 1, CONE_SEGMENTS, 1.0f, 0.0f);
                addTriangle(mesh, apex, current, next);
                addTriangle(mesh, center, next, current);
            }
            return mesh;
        }

        /// A ring of radius one around the x axis, its square tube @p thickness of that radius
        /// to either side of the circle. A tube rather than a flat annulus because a rotation
        /// ring is routinely looked at edge-on, where a flat one would have no width at all.
        [[nodiscard]] OverlayMeshData buildRing(const float thickness)
        {
            OverlayMeshData mesh;
            const float inner = 1.0f - thickness;
            const float outer = 1.0f + thickness;
            for (int segment = 0; segment < RING_SEGMENTS; ++segment)
            {
                const auto point = [&](const int step, const float radius, const float x) {
                    return onCircle(segment + step, RING_SEGMENTS, radius, x);
                };
                addQuad(mesh, point(0, outer, -thickness), point(1, outer, -thickness),
                        point(1, outer, thickness), point(0, outer, thickness));
                addQuad(mesh, point(0, inner, -thickness), point(0, inner, thickness),
                        point(1, inner, thickness), point(1, inner, -thickness));
                addQuad(mesh, point(0, inner, thickness), point(0, outer, thickness),
                        point(1, outer, thickness), point(1, inner, thickness));
                addQuad(mesh, point(0, inner, -thickness), point(1, inner, -thickness),
                        point(1, outer, -thickness), point(0, outer, -thickness));
            }
            return mesh;
        }

        /// The unit square in the local yz plane, translucent inside an opaque border. Both
        /// faces of every quad, since a plane handle is grabbed as readily from behind.
        [[nodiscard]] OverlayMeshData buildPlane()
        {
            OverlayMeshData mesh;
            const auto point = [](const float y, const float z) { return Vector3{0.0f, y, z}; };
            constexpr float borderEnd = 1.0f - PLANE_BORDER;

            addDoubleSidedQuad(mesh, point(0.0f, 0.0f), point(1.0f, 0.0f), point(1.0f, 1.0f), point(0.0f, 1.0f),
                               PLANE_FILL_ALPHA);
            addDoubleSidedQuad(mesh, point(0.0f, 0.0f), point(1.0f, 0.0f), point(1.0f, PLANE_BORDER),
                               point(0.0f, PLANE_BORDER));
            addDoubleSidedQuad(mesh, point(0.0f, borderEnd), point(1.0f, borderEnd), point(1.0f, 1.0f),
                               point(0.0f, 1.0f));
            addDoubleSidedQuad(mesh, point(0.0f, PLANE_BORDER), point(PLANE_BORDER, PLANE_BORDER),
                               point(PLANE_BORDER, borderEnd), point(0.0f, borderEnd));
            addDoubleSidedQuad(mesh, point(borderEnd, PLANE_BORDER), point(1.0f, PLANE_BORDER), point(1.0f, borderEnd),
                               point(borderEnd, borderEnd));
            return mesh;
        }

        /// The matrix taking a mesh's local space to the world, its three columns the images of
        /// the local axes. Assembled by hand because a raylib Matrix names those columns in a
        /// field order no arithmetic here would make obvious.
        [[nodiscard]] Matrix basisMatrix(const Vector3 &x, const Vector3 &y, const Vector3 &z,
                                         const Vector3 &translation)
        {
            Matrix matrix{};
            matrix.m0 = x.x;
            matrix.m1 = x.y;
            matrix.m2 = x.z;
            matrix.m4 = y.x;
            matrix.m5 = y.y;
            matrix.m6 = y.z;
            matrix.m8 = z.x;
            matrix.m9 = z.y;
            matrix.m10 = z.z;
            matrix.m12 = translation.x;
            matrix.m13 = translation.y;
            matrix.m14 = translation.z;
            matrix.m15 = 1.0f;
            return matrix;
        }

        /// Places a unit shape along one of the gizmo's axes: its local x reaches @p length
        /// down that axis and its other two are @p radius across, with its local origin at
        /// @p origin. The axes are a right-handed triple in every rotation the gizmo takes them
        /// in, which is what lets one winding serve all three.
        [[nodiscard]] Matrix placeAlongAxis(const GizmoFrame &frame, const int axisIndex, const float length,
                                            const float radius, const Vector3 &origin)
        {
            return basisMatrix(Vector3Scale(frame.axis[axisIndex], length),
                               Vector3Scale(frame.axis[(axisIndex + 1) % 3], radius),
                               Vector3Scale(frame.axis[(axisIndex + 2) % 3], radius),
                               origin);
        }

        [[nodiscard]] bool isAxisVisible(const GizmoFrame &frame, const int axisIndex)
        {
            return !frame.isDragging() || (frame.activeAxes & (1u << axisIndex)) != 0;
        }
    }

    void GizmoRenderer::initialize()
    {
        auto &renderer = BreadEngine::Renderer::get();

        _effect = renderer.createOverlayEffect(BreadEngine::OverlayEffectDesc{
            .shaderDirectory = "shaders/editor",
            .vertexShader = "gizmo.vsh",
            .pixelShader = "gizmo.psh",
            .parameterSize = sizeof(HandleParameters),
            // The plane handle is translucent and nothing else is, so one blended pipeline
            // serves both. Nothing is depth-tested: a handle standing inside the object it
            // moves is still the handle the pointer has to be able to reach.
            .blend = BreadEngine::OverlayBlendMode::Alpha,
            .depth = BreadEngine::OverlayDepthMode::Disabled,
            // Every shape here is closed, so culling is what keeps a solid's far side from
            // being painted over its near one - there is no depth test to do it.
            .cullBackFaces = true
        });

        _shaft = renderer.createOverlayMesh(buildShaft());
        _cone = renderer.createOverlayMesh(buildCone());
        _cube = renderer.createOverlayMesh(buildCube());
        _plane = renderer.createOverlayMesh(buildPlane());
        _rotationRing = renderer.createOverlayMesh(buildRing(GizmoLayout::ROTATION_RING_THICKNESS));
        _freeMoveRing = renderer.createOverlayMesh(buildRing(GizmoLayout::FREE_MOVE_RING_THICKNESS));
    }

    void GizmoRenderer::shutdown()
    {
        if (!BreadEngine::Renderer::isAlive()) return;

        auto &renderer = BreadEngine::Renderer::get();
        for (auto *mesh : {&_shaft, &_cone, &_cube, &_plane, &_rotationRing, &_freeMoveRing})
        {
            renderer.destroyOverlayMesh(*mesh);
            *mesh = {};
        }
        renderer.destroyOverlayEffect(_effect);
        _effect = {};
    }

    void GizmoRenderer::render(const GizmoFrame &frame)
    {
        if (!_effect.isValid()) return;

        _handles.clear();
        if (frame.mode == GizmoMode::Rotate)
        {
            addRotationRings(frame);
        }
        else
        {
            addAxisHandles(frame);
            addPlaneHandles(frame);
            addFreeMoveRing(frame);
        }

        // Nothing here is depth-tested, so the order the handles are issued in is the only
        // thing deciding which of two overlapping ones is seen. Stable, so handles standing at
        // the same point - the rings, all centred on the origin - keep the order they were
        // added in, and the frame stays identical between runs.
        std::stable_sort(_handles.begin(), _handles.end(),
                         [](const PendingHandle &first, const PendingHandle &second) {
                             return first.distanceFromCamera > second.distanceFromCamera;
                         });

        auto &renderer = BreadEngine::Renderer::get();
        for (const auto &handle : _handles)
        {
            renderer.drawOverlay(BreadEngine::OverlayDrawDesc{
                .mesh = handle.mesh,
                .effect = _effect,
                .parameters = &handle.parameters
            });
        }
    }

    void GizmoRenderer::addHandle(const Matrix &model, const Vector4 color, const Vector3 representativePoint,
                                  const BreadEngine::OverlayMeshHandle mesh, const GizmoFrame &frame)
    {
        if (!mesh.isValid()) return;

        _handles.push_back(PendingHandle{
            .mesh = mesh,
            .parameters = {.model = MatrixToFloatV(model), .color = color},
            .distanceFromCamera = Vector3DistanceSqr(frame.cameraPosition, representativePoint)
        });
    }

    void GizmoRenderer::addAxisHandles(const GizmoFrame &frame)
    {
        const bool isScale = frame.mode == GizmoMode::Scale;
        const float shaftLength = frame.size * (isScale
                                                    ? GizmoLayout::SCALE_SHAFT_LENGTH
                                                    : 1.0f - GizmoLayout::ARROW_LENGTH);
        const float tipLength = frame.size * (isScale ? GizmoLayout::CUBE_HALF_EXTENT : GizmoLayout::ARROW_LENGTH);
        const float tipRadius = frame.size * (isScale ? GizmoLayout::CUBE_HALF_EXTENT : GizmoLayout::ARROW_RADIUS);

        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
        {
            if (!isAxisVisible(frame, axisIndex)) continue;

            const Vector3 &direction = frame.axis[axisIndex];
            addHandle(placeAlongAxis(frame, axisIndex, shaftLength, frame.size * GizmoLayout::SHAFT_RADIUS,
                                     frame.origin),
                      AXIS_COLOR[axisIndex],
                      Vector3Add(frame.origin, Vector3Scale(direction, shaftLength * 0.5f)),
                      _shaft, frame);

            // A cone stands on the end of the shaft while a cube straddles it, which past the
            // shaft's own length is the whole difference between the two modes.
            const float tipOrigin = isScale ? shaftLength + tipLength : shaftLength;
            addHandle(placeAlongAxis(frame, axisIndex, tipLength, tipRadius,
                                     Vector3Add(frame.origin, Vector3Scale(direction, tipOrigin))),
                      AXIS_COLOR[axisIndex],
                      Vector3Add(frame.origin, Vector3Scale(direction, shaftLength + tipLength * 0.5f)),
                      isScale ? _cube : _cone, frame);
        }
    }

    void GizmoRenderer::addPlaneHandles(const GizmoFrame &frame)
    {
        // A plane handle is precisely what a drag underway is not doing: for its duration the
        // gizmo is reduced to the axes being dragged.
        if (frame.isDragging()) return;

        const float offset = frame.size * GizmoLayout::PLANE_OFFSET;
        const float side = frame.size * GizmoLayout::PLANE_SIZE;

        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
        {
            const Vector3 spanned = Vector3Add(frame.axis[(axisIndex + 1) % 3], frame.axis[(axisIndex + 2) % 3]);
            const Vector3 nearCorner = Vector3Add(frame.origin, Vector3Scale(spanned, offset));
            addHandle(placeAlongAxis(frame, axisIndex, 1.0f, side, nearCorner),
                      AXIS_COLOR[axisIndex],
                      Vector3Add(nearCorner, Vector3Scale(spanned, side * 0.5f)),
                      _plane, frame);
        }
    }

    void GizmoRenderer::addRotationRings(const GizmoFrame &frame)
    {
        const float radius = frame.size * GizmoLayout::ROTATION_RING_RADIUS;
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
        {
            if (!isAxisVisible(frame, axisIndex)) continue;

            addHandle(placeAlongAxis(frame, axisIndex, radius, radius, frame.origin),
                      AXIS_COLOR[axisIndex], frame.origin, _rotationRing, frame);
        }
    }

    void GizmoRenderer::addFreeMoveRing(const GizmoFrame &frame)
    {
        // Built in the camera's own basis so that it reads as a circle from anywhere, and added
        // last so that it wins the tie against everything else standing on the origin.
        const float radius = frame.size * GizmoLayout::FREE_MOVE_RING_RADIUS;
        addHandle(basisMatrix(Vector3Scale(frame.cameraForward, radius),
                              Vector3Scale(frame.cameraUp, radius),
                              Vector3Scale(frame.cameraRight, radius),
                              frame.origin),
                  FREE_MOVE_COLOR, frame.origin, _freeMoveRing, frame);
    }
} // namespace BreadEditor
