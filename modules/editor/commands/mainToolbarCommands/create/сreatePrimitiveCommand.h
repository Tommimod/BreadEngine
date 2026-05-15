#pragma once
#include "engine.h"
#include "meshRenderer.h"
#include "commands/command.h"
#include "data/primitives/cubePrimitiveData.h"
#include "uitoolkit/uiInspector.h"

namespace BreadEditor {
    struct CreatePrimitiveCommand : Command
    {
        explicit CreatePrimitiveCommand(Node *parentNode, MeshPrimitiveType primitiveType);

        ~CreatePrimitiveCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        UiInspector *_dataInspector = nullptr;
        Node *_parentNode = nullptr;
        SubscriptionHandle _nodeCreatedSubscription;
        SubscriptionHandle _fileSelectedSubscription;
        SubscriptionHandle _nodeSelectedSubscription;
        MeshPrimitiveType _primitiveType;
        std::thread _thread;
        std::string _inspectorId;

        void onNodeCreated(Node *node);

        void processDataAsync(unsigned int nodeId, MeshPrimitiveData &data);

        void destroyInspector();

        static std::function<void ()> getFunctionByType(unsigned int nodeId, MeshPrimitiveData &data);
    };
} // BreadEditor
