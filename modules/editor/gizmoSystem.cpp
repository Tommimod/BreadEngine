#include "gizmoSystem.h"

namespace BreadEditor {
    void GizmoSystem::recalculateGizmo(BreadEngine::Transform &nodeTransform)
    {
        _transform.translation = nodeTransform.getPosition();
        _transform.rotation = nodeTransform.getRotationQuaternion();
        _transform.scale = nodeTransform.getScale();
        _nodeTransform = &nodeTransform;
    }

    void GizmoSystem::render()
    {
        const auto isTransforming = IsGizmoTransforming();
        if (!isTransforming)
        {
            recalculateGizmo(*_nodeTransform);
        }

        DrawGizmo3D(_mode, &_transform);
        if (!isTransforming)
        {
            return;
        }

        _nodeTransform->setPosition(_transform.translation);
        _nodeTransform->setRotation(_transform.rotation);
        _nodeTransform->setScale(_transform.scale);
    }
} // BreadEditor
