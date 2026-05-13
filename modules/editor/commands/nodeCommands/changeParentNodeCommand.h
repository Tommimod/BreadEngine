#pragma once
#include "engine.h"
#include "commands/command.h"

namespace BreadEditor {
    struct ChangeParentNodeCommand : Command
    {
        explicit ChangeParentNodeCommand(BreadEngine::Node *node, BreadEngine::Node *nextParent);

        ~ChangeParentNodeCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::Node *_node = nullptr;
        BreadEngine::Node *_nextParent = nullptr;
        BreadEngine::Node *_prevParent = nullptr;
    };
} // BreadEditor
