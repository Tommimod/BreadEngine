#include "command.h"

namespace BreadEditor {
    void Command::merge(std::unique_ptr<Command> other)
    {
        other->execute();
        _commands.push_back(std::move(other));
    }

    bool Command::withUndo()
    {
        return false;
    }

    void Command::execute()
    {
    }

    void Command::undo()
    {
        if (_commands.empty()) return;
        const auto size = static_cast<int>(_commands.size());
        for (int i = size - 1; i >= 0; --i)
        {
            _commands[i]->undo();
        }
    }
} // BreadEditor
