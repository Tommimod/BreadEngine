#pragma once
#include "node.h"
#include "commands/command.h"
#include <string>

namespace BreadEditor {
    struct ChangeNameNodeCommand : Command
    {
        explicit ChangeNameNodeCommand(BreadEngine::Node *node, const std::string &newName, const std::string &oldName);

        ~ChangeNameNodeCommand() override = default;

    private:
        BreadEngine::Node *_node = nullptr;
        std::string _newName;
        std::string _oldName;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
