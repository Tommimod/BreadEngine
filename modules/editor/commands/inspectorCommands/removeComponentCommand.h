#pragma once
#include "engine.h"
#include "commands/command.h"

namespace BreadEditor {
    struct RemoveComponentCommand : Command
    {
        explicit RemoveComponentCommand(BreadEngine::Node *node, std::type_index type);

        ~RemoveComponentCommand() override = default;

    private:
        BreadEngine::Node *_node = nullptr;
        std::unique_ptr<BreadEngine::Component> _originalComponent = nullptr;
        std::type_index _type;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
