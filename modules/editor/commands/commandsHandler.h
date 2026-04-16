#pragma once
#include <cstddef>
#include <memory>
#include <vector>
#include "command.h"

namespace BreadEditor {
    class CommandsHandler
    {
    public:
        static void execute(std::unique_ptr<Command> command);

        static void undo();

    private:
        static std::byte _historySize;

        static std::vector<std::unique_ptr<Command> > _history;
    };
} // BreadEditor
