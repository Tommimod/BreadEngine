#pragma once
#include "commands/command.h"
#include "commands/nodeCommands/createNodeCommand.h"

namespace BreadEditor {
    struct CreateEmptyNodeCommand : Command
    {
        explicit CreateEmptyNodeCommand(BreadEngine::Node *parentNode);

        ~CreateEmptyNodeCommand() override;

        void execute() override;

        void undo() override;

        bool withUndo() override { return true; }

    private:
        BreadEngine::Node *_engineNode = nullptr;
    };
} // BreadEditor
