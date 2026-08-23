#pragma once
#include <cstdint>
#include <vector>

#include "raylib.h"
// After raylib.h, which is the only order in which raymath's unguarded Vector and Matrix
// declarations agree with it. float16 is what a matrix crosses into a constant buffer as.
#include "raymath.h"
#include "rendering/overlayTypes.h"
#include "rendering/renderHandles.h"

namespace BreadEditor {
    /// What the handles under the cursor do to the selection. Exclusive: one gizmo stands on
    /// screen at a time and every handle it draws belongs to that one.
    enum class GizmoMode : uint8_t
    {
        Translate = 0,
        Rotate,
        Scale
    };

    /// Whether the handles follow the world's axes or the selection's own.
    enum class GizmoSpace : uint8_t
    {
        World = 0,
        Local
    };

    /// Which axes a handle acts on. A plane handle owns two of them and the free-move ring
    /// owns all three, so the set is a mask rather than an index.
    enum GizmoAxes : uint8_t
    {
        GIZMO_AXIS_NONE = 0,
        GIZMO_AXIS_X = 1 << 0,
        GIZMO_AXIS_Y = 1 << 1,
        GIZMO_AXIS_Z = 1 << 2,
        GIZMO_AXIS_ALL = GIZMO_AXIS_X | GIZMO_AXIS_Y | GIZMO_AXIS_Z
    };

    /**
     * The gizmo's proportions. Everything but the first is a fraction of the gizmo's own world
     * size, so the whole thing scales as one - and both halves of the gizmo read them from
     * here, because a handle drawn where it cannot be grabbed is the failure this shares them
     * to prevent.
     */
    namespace GizmoLayout {
        /// The gizmo's world size as a fraction of its distance from the eye, which is what
        /// keeps it the same size on screen wherever the camera stands.
        constexpr float SCREEN_FRACTION = 0.15f;

        constexpr float ARROW_LENGTH = 0.15f;
        constexpr float ARROW_RADIUS = 0.05f;
        constexpr float SHAFT_RADIUS = 0.006f;
        constexpr float CUBE_HALF_EXTENT = 0.05f;
        /// A scale handle's shaft stops at the near face of its cube rather than at the axis's
        /// full length, so the cube reads as the end of the shaft rather than a bead on it.
        constexpr float SCALE_SHAFT_LENGTH = 1.0f - 2.0f * CUBE_HALF_EXTENT;

        /// Where a plane handle's near corner sits along each of the two axes it spans, and
        /// how far it reaches from there.
        constexpr float PLANE_OFFSET = 0.3f;
        constexpr float PLANE_SIZE = 0.15f;

        constexpr float ROTATION_RING_RADIUS = 1.0f;
        constexpr float FREE_MOVE_RING_RADIUS = 0.1f;
        /// Tube thickness as a fraction of each ring's own radius. Two numbers rather than one
        /// because a ring is sized by a single uniform scale, which carries its tube with it:
        /// the free-move ring is a tenth of the radius, so it takes ten times the relative
        /// thickness to come out the same width on screen.
        constexpr float ROTATION_RING_THICKNESS = 0.006f;
        constexpr float FREE_MOVE_RING_THICKNESS = 0.06f;
    }

    /**
     * One frame of the gizmo: where it stands, how it is oriented, and how much of it the drag
     * currently underway leaves visible. GizmoSystem builds it and is also the only thing that
     * hit-tests against it, so the shape drawn and the shape grabbed cannot drift apart.
     */
    struct GizmoFrame
    {
        Vector3 origin{};
        /// The directions the three axis handles point along - the world's, or the selection's
        /// own under GizmoSpace::Local. Always a right-handed, orthonormal triple, which is
        /// what lets the meshes be wound once in a local space and used for all three.
        Vector3 axis[3]{};
        /// The length of one axis handle in world units; every other measure is a fraction of
        /// it through GizmoLayout.
        float size = 1.0f;
        GizmoMode mode = GizmoMode::Translate;
        /// The axes the drag underway is constrained to. Every other handle is hidden for its
        /// duration, which is what keeps the one being dragged readable; GIZMO_AXIS_NONE while
        /// nothing is being dragged, and then the whole gizmo draws.
        uint8_t activeAxes = GIZMO_AXIS_NONE;

        /// The eye the frame is seen from. Handles are drawn back to front against it, since
        /// the pass they draw through has no depth test to order them.
        Vector3 cameraPosition{};
        /// The camera's own basis. The free-move ring is built in the plane the last two span
        /// so that it always faces the viewer, and a drag with no single axis to follow - free
        /// move, and rotation - is measured against them.
        Vector3 cameraForward{};
        Vector3 cameraRight{};
        Vector3 cameraUp{};

        [[nodiscard]] bool isDragging() const { return activeAxes != GIZMO_AXIS_NONE; }
    };

    /**
     * The transform handles, drawn through the overlay channel from a shader pair the editor
     * ships. Owns one pipeline and the five unit-space shapes every handle is an instance of;
     * what a handle *is* lives entirely in the model matrix and colour handed to each draw.
     */
    class GizmoRenderer
    {
    public:
        void initialize();

        void shutdown();

        /// Draws into the open overlay pass, so it belongs between beginOverlay and endOverlay.
        void render(const GizmoFrame &frame);

    private:
        /// Mirrors gizmo.vsh's own constant block: one handle's place in the world and the
        /// colour it is painted. The matrix is a float16 rather than a Matrix because the two
        /// are transposes of each other in memory, and the shader reads what was uploaded.
        struct HandleParameters
        {
            float16 model{};
            Vector4 color{};
        };

        /// One handle, resolved to the draw that will produce it. Collected before anything is
        /// issued so the whole gizmo can be ordered back to front in one place.
        struct PendingHandle
        {
            BreadEngine::OverlayMeshHandle mesh;
            HandleParameters parameters;
            /// Squared distance from the eye to the point that stands for this handle.
            float distanceFromCamera = 0.0f;
        };

        void addHandle(const Matrix &model, Vector4 color, Vector3 representativePoint,
                       BreadEngine::OverlayMeshHandle mesh, const GizmoFrame &frame);

        void addAxisHandles(const GizmoFrame &frame);

        void addPlaneHandles(const GizmoFrame &frame);

        void addRotationRings(const GizmoFrame &frame);

        void addFreeMoveRing(const GizmoFrame &frame);

        BreadEngine::OverlayEffectHandle _effect{};

        /// A square prism from the origin to +x: the body of a translate or scale handle.
        BreadEngine::OverlayMeshHandle _shaft{};
        /// The arrowhead a translate handle ends in, its base at the origin and its apex at +x.
        BreadEngine::OverlayMeshHandle _cone{};
        /// The block a scale handle ends in, centred on the origin.
        BreadEngine::OverlayMeshHandle _cube{};
        /// The two-axis handle: a translucent square in the local yz plane with an opaque
        /// border, spanning the unit square from the origin.
        BreadEngine::OverlayMeshHandle _plane{};
        BreadEngine::OverlayMeshHandle _rotationRing{};
        BreadEngine::OverlayMeshHandle _freeMoveRing{};

        /// Cleared and refilled every frame rather than rebuilt, so drawing the gizmo costs no
        /// allocation once it has been drawn once.
        std::vector<PendingHandle> _handles;
    };
} // namespace BreadEditor
