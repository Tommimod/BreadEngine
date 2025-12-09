#include "gizmoSystem.h"

namespace BreadEditor {
    void GizmoSystem::Draw(const BreadEngine::Transform &nodeTransform)
    {
        _transform.translation = nodeTransform.getPosition();
        _transform.rotation = nodeTransform.getRotationQuaternion();
        _transform.scale = nodeTransform.getScale();

        DrawGizmo3D(_mode, &_transform);
    }
} // BreadEditor