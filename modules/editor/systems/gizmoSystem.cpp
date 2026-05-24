#include "gizmoSystem.h"

#include "raymath.h"
#include "../editor.h"
#include "commands/commandsHandler.h"
#include "commands/nodeCommands/changeNodeTransformCommand.h"
#include "tracy/Tracy.hpp"

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
        ZoneScoped;
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
            if (isPrevTransformInitialized())
            {
                CommandsHandler::execute(std::make_unique<ChangeNodeTransformCommand>(_nodeTransform->getOwner(), _transform.rotation, _transform.translation, _transform.scale, _prevTransform.rotation, _prevTransform.translation, _prevTransform.scale));
                _prevTransform = ::Transform{};
            }

            recalculateGizmo(*_nodeTransform);
            return;
        }

        if (!isPrevTransformInitialized())
        {
            _prevTransform = _transform;
        }

        _nodeTransform->setPosition(_transform.translation);
        _nodeTransform->setRotation(_transform.rotation);
        _nodeTransform->setScale(_transform.scale);
    }

    bool GizmoSystem::isPrevTransformInitialized() const
    {
        return !Vector3Equals(_prevTransform.translation, Vector3{}) && !Vector4Equals(_prevTransform.rotation, Quaternion{}) && !Vector3Equals(_prevTransform.scale, Vector3{});
    }
} // BreadEditor
