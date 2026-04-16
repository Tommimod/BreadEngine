#pragma once
#include "commands/command.h"

namespace BreadEditor {
    struct OpenProjectCommand : Command
    {
    private:
        bool withUndo() override { return false; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
