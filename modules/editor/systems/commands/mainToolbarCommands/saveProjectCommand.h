#pragma once
#include "systems/commands/command.h"

namespace BreadEditor {
    struct SaveProjectCommand : Command
    {
    private:
        bool withUndo() override {return false;}

        void execute() override;

        void undo() override;
    };
} // BreadEditor
