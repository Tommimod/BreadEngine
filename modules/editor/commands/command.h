#pragma once
#include <memory>
#include <vector>

namespace BreadEditor {
    struct Command
    {
        virtual ~Command() = default;

        virtual bool withUndo() = 0;

        virtual void execute() = 0;

        virtual void undo() = 0;

    protected:
        void merge(std::unique_ptr<Command> other);

    private:
        friend class CommandsHandler;
        std::vector<std::unique_ptr<Command> > _commands;
    };
} // BreadEditor
