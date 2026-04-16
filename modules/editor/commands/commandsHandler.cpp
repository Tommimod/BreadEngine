#include "commandsHandler.h"

#include <algorithm>

namespace BreadEditor {
    std::byte CommandsHandler::_historySize{32};
    std::vector<std::unique_ptr<Command> > CommandsHandler::_history{};

    void CommandsHandler::execute(std::unique_ptr<Command> command)
    {
        command->execute();
        if (!command->withUndo()) return;

        if (_history.size() == static_cast<std::vector<std::unique_ptr<Command> >::size_type>(_historySize))
        {
            _history.erase(std::ranges::find(_history.begin(), _history.end(), _history.front()));
        }

        _history.push_back(std::move(command));
    }

    void CommandsHandler::undo()
    {
        if (_history.empty()) return;
        _history.back()->undo();
        _history.erase(std::ranges::find(_history.begin(), _history.end(), _history.back()));
    }
} // BreadEditor
