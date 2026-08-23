#include "gizmoSystem.h"

#include <cmath>

#include "raymath.h"
#include "../editor.h"
#include "commands/commandsHandler.h"
#include "commands/nodeCommands/changeNodeTransformCommand.h"
#include "tracy/Tracy.hpp"

namespace BreadEditor {
    namespace {
        constexpr Vector3 WORLD_AXIS[3]{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};

        /// How many points a rotation ring is grabbed by. Fewer than it is drawn with: each one
        /// stands for a sphere covering its whole arc, so the count sets the grab tolerance and
        /// a finer ring would only make it harder to catch.
        constexpr int RING_HIT_POINTS = 36;

        [[nodiscard]] uint8_t axisBit(const int axisIndex) { return static_cast<uint8_t>(1u << axisIndex); }

        /// Whether @p ray reaches a box standing in the gizmo's own axes. The ray is taken into
        /// those axes rather than the box out of them, which is what lets raylib's axis-aligned
        /// test serve a gizmo that may be turned any way at all.
        [[nodiscard]] bool hitsOrientedBox(const GizmoFrame &frame, const Ray &ray, const Vector3 &center,
                                           const Vector3 &halfSize)
        {
            const Vector3 toRayOrigin = Vector3Subtract(ray.position, center);
            const Ray localRay{
                .position = {
                    Vector3DotProduct(toRayOrigin, frame.axis[0]),
                    Vector3DotProduct(toRayOrigin, frame.axis[1]),
                    Vector3DotProduct(toRayOrigin, frame.axis[2])
                },
                .direction = {
                    Vector3DotProduct(ray.direction, frame.axis[0]),
                    Vector3DotProduct(ray.direction, frame.axis[1]),
                    Vector3DotProduct(ray.direction, frame.axis[2])
                }
            };
            const BoundingBox box{.min = Vector3Negate(halfSize), .max = halfSize};
            return GetRayCollisionBox(localRay, box).hit;
        }

        /// The whole length of one axis handle is grabbable, and as wide as its widest part -
        /// so an arrow is caught by its shaft as readily as by its head.
        [[nodiscard]] bool hitsAxis(const GizmoFrame &frame, const int axisIndex, const Ray &ray)
        {
            float halfSize[3];
            halfSize[axisIndex] = frame.size * 0.5f;
            halfSize[(axisIndex + 1) % 3] = frame.size * GizmoLayout::ARROW_RADIUS;
            halfSize[(axisIndex + 2) % 3] = halfSize[(axisIndex + 1) % 3];

            const Vector3 center = Vector3Add(frame.origin, Vector3Scale(frame.axis[axisIndex], halfSize[axisIndex]));
            return hitsOrientedBox(frame, ray, center, Vector3{halfSize[0], halfSize[1], halfSize[2]});
        }

        [[nodiscard]] bool hitsPlane(const GizmoFrame &frame, const int axisIndex, const Ray &ray)
        {
            const Vector3 &first = frame.axis[(axisIndex + 1) % 3];
            const Vector3 &second = frame.axis[(axisIndex + 2) % 3];
            const float offset = frame.size * GizmoLayout::PLANE_OFFSET;
            const float side = frame.size * GizmoLayout::PLANE_SIZE;

            const Vector3 nearCorner = Vector3Add(frame.origin,
                                                  Vector3Scale(Vector3Add(first, second), offset));
            const Vector3 alongFirst = Vector3Add(nearCorner, Vector3Scale(first, side));
            const Vector3 farCorner = Vector3Add(alongFirst, Vector3Scale(second, side));
            const Vector3 alongSecond = Vector3Add(nearCorner, Vector3Scale(second, side));
            return GetRayCollisionQuad(ray, nearCorner, alongFirst, farCorner, alongSecond).hit;
        }

        /// A ring is grabbed as a string of spheres laid along it, each wide enough to cover the
        /// arc between it and the next - which is a circle's own thickness plus a tolerance,
        /// rather than the tube the ring is drawn as.
        [[nodiscard]] bool hitsRing(const GizmoFrame &frame, const int axisIndex, const Ray &ray)
        {
            const Vector3 &first = frame.axis[(axisIndex + 1) % 3];
            const Vector3 &second = frame.axis[(axisIndex + 2) % 3];
            const float radius = frame.size * GizmoLayout::ROTATION_RING_RADIUS;
            const float tolerance = radius * std::sin(PI / static_cast<float>(RING_HIT_POINTS));

            for (int point = 0; point < RING_HIT_POINTS; ++point)
            {
                const float angle = 2.0f * PI * static_cast<float>(point) / static_cast<float>(RING_HIT_POINTS);
                Vector3 onRing = Vector3Add(frame.origin, Vector3Scale(first, std::sin(angle) * radius));
                onRing = Vector3Add(onRing, Vector3Scale(second, std::cos(angle) * radius));

                if (GetRayCollisionSphere(ray, onRing, tolerance).hit) return true;
            }
            return false;
        }

        [[nodiscard]] bool hitsFreeMove(const GizmoFrame &frame, const Ray &ray)
        {
            return GetRayCollisionSphere(ray, frame.origin, frame.size * GizmoLayout::FREE_MOVE_RING_RADIUS).hit;
        }
    }

    void GizmoSystem::initialize()
    {
        _renderer.initialize();
    }

    void GizmoSystem::shutdown()
    {
        _renderer.shutdown();
    }

    void GizmoSystem::recalculateGizmo(BreadEngine::Transform &nodeTransform)
    {
        _transform.translation = nodeTransform.getPosition();
        _transform.rotation = nodeTransform.getRotationQuaternion();
        _transform.scale = nodeTransform.getScale();
        _nodeTransform = &nodeTransform;
    }

    void GizmoSystem::render(const Camera3D &camera)
    {
        ZoneScoped;
        if (_nodeTransform == nullptr) return;
        if (_viewportWindow == nullptr)
        {
            _viewportWindow = &Editor::getInstance().mainWindow.getViewportWindow();
        }

        // Outside a drag the node is the authority: the inspector and the undo stack both write
        // straight through it, and the handles have to follow.
        if (_activeAxes == GIZMO_AXIS_NONE) recalculateGizmo(*_nodeTransform);

        processInput(buildFrame(camera));
        // Built a second time because the drag the first one resolved is what moves the gizmo
        // this one draws.
        _renderer.render(buildFrame(camera));
    }

    GizmoFrame GizmoSystem::buildFrame(const Camera3D &camera) const
    {
        GizmoFrame frame;
        frame.origin = _transform.translation;
        frame.mode = _mode;
        frame.activeAxes = _activeAxes;

        frame.cameraPosition = camera.position;
        frame.cameraForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        frame.cameraRight = Vector3Normalize(Vector3CrossProduct(frame.cameraForward, camera.up));
        frame.cameraUp = Vector3CrossProduct(frame.cameraRight, frame.cameraForward);
        frame.size = Vector3Distance(camera.position, frame.origin) * GizmoLayout::SCREEN_FRACTION;

        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
        {
            frame.axis[axisIndex] = _space == GizmoSpace::Local
                                        ? Vector3Normalize(Vector3RotateByQuaternion(WORLD_AXIS[axisIndex],
                                                                                     _transform.rotation))
                                        : WORLD_AXIS[axisIndex];
        }
        return frame;
    }

    void GizmoSystem::processInput(const GizmoFrame &frame)
    {
        const Ray ray = _viewportWindow->getMouseRay();

        if (_activeAxes != GIZMO_AXIS_NONE)
        {
            // A drag that wanders off the viewport is still that drag, so only the grab below
            // asks where the pointer is.
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) applyDrag(frame, ray);
            else endDrag();
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && _viewportWindow->isMouseOver()) beginDrag(frame, ray);
    }

    void GizmoSystem::beginDrag(const GizmoFrame &frame, const Ray &ray)
    {
        uint8_t grabbed = GIZMO_AXIS_NONE;

        if (_mode == GizmoMode::Rotate)
        {
            for (int axisIndex = 0; axisIndex < 3 && grabbed == GIZMO_AXIS_NONE; ++axisIndex)
            {
                if (hitsRing(frame, axisIndex, ray)) grabbed = axisBit(axisIndex);
            }
        }
        else if (hitsFreeMove(frame, ray))
        {
            grabbed = GIZMO_AXIS_ALL;
        }
        else
        {
            for (int axisIndex = 0; axisIndex < 3 && grabbed == GIZMO_AXIS_NONE; ++axisIndex)
            {
                if (hitsAxis(frame, axisIndex, ray)) grabbed = axisBit(axisIndex);
                // A plane handle is named by the axis it stands across and acts on the other
                // two, which is the whole of what makes it a plane.
                else if (hitsPlane(frame, axisIndex, ray))
                {
                    grabbed = static_cast<uint8_t>(axisBit((axisIndex + 1) % 3) | axisBit((axisIndex + 2) % 3));
                }
            }
        }

        if (grabbed == GIZMO_AXIS_NONE) return;

        _activeAxes = grabbed;
        _dragStart = _transform;
        _beforeDrag = _transform;
        _dragStartPointer = pointerInWorld(frame, ray);
    }

    void GizmoSystem::applyDrag(const GizmoFrame &frame, const Ray &ray)
    {
        const Vector3 pointer = pointerInWorld(frame, ray);
        const Vector3 delta = Vector3Subtract(pointer, _dragStartPointer);

        switch (_mode)
        {
        case GizmoMode::Translate:
            {
                Vector3 moved = _dragStart.translation;
                if (_activeAxes == GIZMO_AXIS_ALL)
                {
                    // Free move follows the screen rather than any axis, so the pointer's own
                    // two directions are what it is resolved against.
                    moved = Vector3Add(moved, Vector3Project(delta, frame.cameraRight));
                    moved = Vector3Add(moved, Vector3Project(delta, frame.cameraUp));
                }
                else
                {
                    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
                    {
                        if ((_activeAxes & axisBit(axisIndex)) == 0) continue;
                        moved = Vector3Add(moved, Vector3Project(delta, frame.axis[axisIndex]));
                    }
                }
                _transform.translation = moved;
            }
            break;
        case GizmoMode::Scale:
            {
                if (_activeAxes == GIZMO_AXIS_ALL)
                {
                    const float amount = Vector3DotProduct(delta, frame.cameraRight) +
                                         Vector3DotProduct(delta, frame.cameraUp);
                    _transform.scale = Vector3AddValue(_dragStart.scale, amount);
                    break;
                }

                float components[3]{_dragStart.scale.x, _dragStart.scale.y, _dragStart.scale.z};
                for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
                {
                    if ((_activeAxes & axisBit(axisIndex)) == 0) continue;
                    // How far the pointer travelled along the handle, which is the one measure
                    // that means the same thing whichever way the handle is turned.
                    components[axisIndex] += Vector3DotProduct(delta, frame.axis[axisIndex]);
                }
                _transform.scale = Vector3{components[0], components[1], components[2]};
            }
            break;
        case GizmoMode::Rotate:
            {
                const float angle = Clamp(Vector3DotProduct(delta, Vector3Add(frame.cameraRight, frame.cameraUp)),
                                          -2.0f * PI, 2.0f * PI);
                Quaternion rotated = _dragStart.rotation;
                for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
                {
                    if ((_activeAxes & axisBit(axisIndex)) == 0) continue;
                    rotated = QuaternionMultiply(QuaternionFromAxisAngle(frame.axis[axisIndex], angle), rotated);
                }
                _transform.rotation = rotated;

                // A rotation turns the axes it is measured against, so each frame starts over
                // from where the last one left off rather than from where the drag began.
                _dragStart = _transform;
                _dragStartPointer = pointer;
            }
            break;
        }

        _nodeTransform->setPosition(_transform.translation);
        _nodeTransform->setRotation(_transform.rotation);
        _nodeTransform->setScale(_transform.scale);
    }

    void GizmoSystem::endDrag()
    {
        _activeAxes = GIZMO_AXIS_NONE;

        // A click that caught a handle without moving it is not an edit, and does not belong on
        // the undo stack.
        if (Vector3Equals(_transform.translation, _beforeDrag.translation) &&
            Vector4Equals(_transform.rotation, _beforeDrag.rotation) &&
            Vector3Equals(_transform.scale, _beforeDrag.scale))
        {
            return;
        }

        CommandsHandler::execute(std::make_unique<ChangeNodeTransformCommand>(
            _nodeTransform->getOwner(),
            _transform.rotation, _transform.translation, _transform.scale,
            _beforeDrag.rotation, _beforeDrag.translation, _beforeDrag.scale));
    }

    Vector3 GizmoSystem::pointerInWorld(const GizmoFrame &frame, const Ray &ray)
    {
        const float distance = Vector3Distance(frame.cameraPosition, frame.origin);
        return Vector3Add(ray.position, Vector3Scale(ray.direction, distance));
    }
} // BreadEditor
