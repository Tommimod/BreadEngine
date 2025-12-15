#include "gizmoSystem.h"

namespace BreadEditor {
    void GizmoSystem::recalculateGizmo(const BreadEngine::Transform &nodeTransform)
    {
        _transform.translation = nodeTransform.getPosition();
        _transform.rotation = nodeTransform.getRotationQuaternion();
        _transform.scale = nodeTransform.getScale();
    }

    void GizmoSystem::render()
    {
        DrawGizmo3D(_mode, &_transform);
    }
} // BreadEditor
