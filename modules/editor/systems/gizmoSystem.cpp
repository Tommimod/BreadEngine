#include "gizmoSystem.h"
#include "../editor.h"

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
        if (_viewportWindow == nullptr)
        {
            _viewportWindow = &Editor::getInstance().mainWindow.getViewportWindow();
        }

        const auto viewportSize = _viewportWindow->getViewportSize();
        const auto isTransforming = DrawGizmo3DViewport(_mode, &_transform,
                                                        _viewportWindow->getMousePosition(), static_cast<int>(viewportSize.width),
                                                        static_cast<int>(viewportSize.height));
        if (!isTransforming)
        {
            recalculateGizmo(*_nodeTransform);
            return;
        }

        _nodeTransform->setPosition(_transform.translation);
        _nodeTransform->setRotation(_transform.rotation);
        _nodeTransform->setScale(_transform.scale);
    }
} // BreadEditor
