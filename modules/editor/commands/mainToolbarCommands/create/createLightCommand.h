#pragma once
#include "node.h"
#include "r3d_lighting.h"
#include "commands/command.h"

namespace BreadEditor {
    struct CreateLightCommand : Command
    {
        explicit CreateLightCommand(BreadEngine::Node *parentNode, R3D_LightType lightType);

        ~CreateLightCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::Node *_parentNode = nullptr;
        BreadEngine::SubscriptionHandle _nodeCreatedSubscription;
        R3D_LightType _lightType;

        void onNodeCreated(BreadEngine::Node *node);
    };
} // BreadEditor
