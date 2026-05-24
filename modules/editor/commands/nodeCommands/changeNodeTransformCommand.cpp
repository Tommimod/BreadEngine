#include "changeNodeTransformCommand.h"

#include "transform.h"

namespace BreadEditor {
    ChangeNodeTransformCommand::ChangeNodeTransformCommand(BreadEngine::Node *node, const Quaternion &newRotation, const Vector3 &newPosition, const Vector3 &newScale, const Quaternion &prevRotation, const Vector3 &prevPosition, const Vector3 &prevScale)
    {
        _node = node;
        _newRotation = newRotation;
        _newPosition = newPosition;
        _newScale = newScale;

        _prevRotation = prevRotation;
        _prevPosition = prevPosition;
        _prevScale = prevScale;
    }

    void ChangeNodeTransformCommand::execute()
    {
        auto &transform = _node->get<BreadEngine::Transform>();
        transform.setLocalRotation(_newRotation);
        transform.setLocalPosition(_newPosition);
        transform.setLocalScale(_newScale);
    }

    void ChangeNodeTransformCommand::undo()
    {
        auto &transform = _node->get<BreadEngine::Transform>();
        transform.setLocalRotation(_prevRotation);
        transform.setLocalPosition(_prevPosition);
        transform.setLocalScale(_prevScale);
    }
} // BreadEditor
