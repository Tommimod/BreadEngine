#pragma once
#include "raylib.h"
#include "transform.h"
#include "../rendering/gizmoRenderer.h"
#include "../windows/viewportWindow.h"

namespace BreadEditor {
    /**
     * The transform handles: what they look like, what the pointer is over, and what dragging
     * one does to the selected node. Owns the renderer that draws them because the shape drawn
     * and the shape hit-tested are the same shape, described once in GizmoLayout.
     */
    class GizmoSystem
    {
    public:
        void initialize();

        void shutdown();

        void setMode(const GizmoMode mode) { _mode = mode; }

        void setSpace(const GizmoSpace space) { _space = space; }

        /// Points the gizmo at the node it will act on, and takes the transform it starts from.
        void recalculateGizmo(BreadEngine::Transform &nodeTransform);

        /// Resolves a drag and draws the result, so it belongs inside the overlay pass.
        /// @param camera the one the frame was rendered through, which is what the handles are
        /// sized and oriented against.
        void render(const Camera3D &camera);

    private:
        [[nodiscard]] GizmoFrame buildFrame(const Camera3D &camera) const;

        void processInput(const GizmoFrame &frame);

        /// Grabs whatever handle @p ray reaches, and records what a drag of it starts from.
        void beginDrag(const GizmoFrame &frame, const Ray &ray);

        void applyDrag(const GizmoFrame &frame, const Ray &ray);

        /// Hands the whole drag to the undo stack as one command, and lets go of the handle.
        void endDrag();

        /// Where the pointer sits on the plane through the gizmo that faces the camera - the
        /// one surface a drag of any handle can be measured on whatever it is constrained to.
        [[nodiscard]] static Vector3 pointerInWorld(const GizmoFrame &frame, const Ray &ray);

        GizmoMode _mode = GizmoMode::Translate;
        GizmoSpace _space = GizmoSpace::World;

        /// The transform the handles show, and the one a drag writes into before it reaches
        /// the node. Reread from the node every frame that is not a drag, so a transform
        /// changed in the inspector moves the gizmo with it.
        ::Transform _transform{};
        /// The transform the drag underway is applied to. Rotation resets it every frame,
        /// because a rotation accumulates and its axes turn under it as it does.
        ::Transform _dragStart{};
        /// The transform as it stood when the drag began, which is the state undo returns to.
        /// Distinct from _dragStart for exactly the reason above.
        ::Transform _beforeDrag{};
        Vector3 _dragStartPointer{};
        /// The axes the drag underway is constrained to, and the fact that one is underway at
        /// all - no handle is grabbed without constraining at least one axis.
        uint8_t _activeAxes = GIZMO_AXIS_NONE;

        BreadEngine::Transform *_nodeTransform = nullptr;
        ViewportWindow *_viewportWindow = nullptr;
        GizmoRenderer _renderer;
    };
} // namespace BreadEditor
