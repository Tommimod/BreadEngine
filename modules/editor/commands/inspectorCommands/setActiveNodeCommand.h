#pragma once
#include "node.h"
#include "commands/command.h"

namespace BreadEditor {
    struct SetActiveNodeCommand : Command
    {
        explicit SetActiveNodeCommand(BreadEngine::Node *node, bool nextState);

        ~SetActiveNodeCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::Node *_node = nullptr;
        bool _nextState;
    };
} // BreadEditor
