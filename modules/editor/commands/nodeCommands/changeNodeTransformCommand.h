#pragma once
#include "node.h"
#include "commands/command.h"

namespace BreadEditor {
    struct ChangeNodeTransformCommand : Command
    {
        explicit ChangeNodeTransformCommand(BreadEngine::Node *node, const Quaternion &newRotation, const Vector3 &newPosition, const Vector3 &newScale, const Quaternion &prevRotation, const Vector3 &prevPosition, const Vector3 &prevScale);

        ~ChangeNodeTransformCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::Node *_node = nullptr;
        Quaternion _prevRotation;
        Vector3 _prevPosition;
        Vector3 _prevScale;

        Quaternion _newRotation;
        Vector3 _newPosition;
        Vector3 _newScale;
    };
} // BreadEditor
