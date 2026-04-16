#pragma once
#include "engine.h"
#include "commands/command.h"

namespace BreadEditor {
    struct AddComponentCommand : Command
    {
        explicit AddComponentCommand(BreadEngine::Node *node, const std::string &componentType);

        ~AddComponentCommand() override = default;

    private:
        BreadEngine::Node *_node = nullptr;
        std::string _componentType;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
