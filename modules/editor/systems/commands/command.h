#pragma once

namespace BreadEditor {
    struct Command
    {
        virtual ~Command() = default;

    private:
        friend class CommandsHandler;

        virtual bool withUndo() = 0;

        virtual void execute() = 0;

        virtual void undo() = 0;
    };
} // BreadEditor
