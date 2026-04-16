#pragma once
#include "engine.h"
#include "commands/command.h"

namespace BreadEditor {
    struct CreateNodeCommand : Command
    {
        explicit CreateNodeCommand(BreadEngine::Node *parentNode);

        explicit CreateNodeCommand(BreadEngine::Node *parentNode, YAML::Node data);

        ~CreateNodeCommand() override = default;

    private:
        BreadEngine::Node *_createdNode = nullptr;
        BreadEngine::Node *_parentNode = nullptr;
        YAML::Node _data{};
        bool _withData = false;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
