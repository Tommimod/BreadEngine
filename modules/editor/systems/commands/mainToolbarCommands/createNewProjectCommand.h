#pragma once
#include "systems/commands/command.h"

namespace BreadEditor {
    struct CreateNewProjectCommand : Command
    {
        CreateNewProjectCommand();

        ~CreateNewProjectCommand() override;

    private:
        void execute() override;

        void undo() override;

        bool withUndo() override { return false; }
    };
} // BreadEditor
