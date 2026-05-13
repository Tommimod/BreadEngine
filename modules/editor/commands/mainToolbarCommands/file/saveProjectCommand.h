#pragma once
#include "commands/command.h"

namespace BreadEditor {
    struct SaveProjectCommand : Command
    {
        bool withUndo() override {return false;}

        void execute() override;

        void undo() override;
    };
} // BreadEditor
