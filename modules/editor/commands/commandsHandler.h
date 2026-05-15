#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>
#include "command.h"

namespace BreadEditor {
    class CommandsHandler
    {
    public:
        static void execute(std::unique_ptr<Command> command);

        static void undo();

        static void addFunction(std::function<void() > func);

        static void update();

    private:
        static std::vector<std::function<void() > > _functions;
        static std::byte _historySize;

        static std::vector<std::unique_ptr<Command> > _history;
    };
} // BreadEditor
