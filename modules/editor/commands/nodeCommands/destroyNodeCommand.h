#pragma once
#include "engine.h"
#include "commands/command.h"

namespace BreadEditor {
    struct DestroyNodeCommand : Command
    {
        explicit DestroyNodeCommand(BreadEngine::Node *node);

        ~DestroyNodeCommand() override = default;

    private:
        BreadEngine::Node *_node = nullptr;
        YAML::Node _data;
        unsigned int _parentId = INT_MAX;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
