#pragma once
#include "systems/commands/command.h"

namespace BreadEditor {
    struct ReopenLastProjectCommand : Command
    {
    private:
        bool withUndo() override {return false;}

        void execute() override;

        void undo() override;
    };
} // BreadEditor