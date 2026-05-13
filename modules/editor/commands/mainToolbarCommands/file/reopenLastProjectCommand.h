#pragma once
#include "commands/command.h"

namespace BreadEditor {
    struct ReopenLastProjectCommand : Command
    {
        bool withUndo() override {return false;}

        void execute() override;

        void undo() override;
    };
} // BreadEditor