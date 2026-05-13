#pragma once
#include "commands/command.h"

namespace BreadEditor {
    struct CreateNewProjectCommand : Command
    {
        CreateNewProjectCommand();

        ~CreateNewProjectCommand() override;

        void execute() override;

        void undo() override;

        bool withUndo() override { return false; }
    };
} // BreadEditor
