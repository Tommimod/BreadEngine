#pragma once
#include "node.h"
#include "commands/command.h"
#include "rendering/renderTypes.h"

namespace BreadEditor {
    struct CreateLightCommand : Command
    {
        explicit CreateLightCommand(BreadEngine::Node *parentNode, BreadEngine::LightType lightType);

        ~CreateLightCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::Node *_parentNode = nullptr;
        BreadEngine::SubscriptionHandle _nodeCreatedSubscription;
        BreadEngine::LightType _lightType;

        void onNodeCreated(BreadEngine::Node *node);
    };
} // BreadEditor
