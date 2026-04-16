#pragma once
#include "node.h"
#include "commands/command.h"

namespace BreadEditor {
    struct SetActiveNodeCommand : Command
    {
        explicit SetActiveNodeCommand(BreadEngine::Node *node, bool nextState);

        ~SetActiveNodeCommand() override = default;

    private:
        BreadEngine::Node *_node = nullptr;
        bool _nextState;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
