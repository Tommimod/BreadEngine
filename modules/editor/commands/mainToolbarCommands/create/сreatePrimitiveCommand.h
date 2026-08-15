#pragma once
#include <memory>

#include "engine.h"
#include "commands/command.h"
#include "data/primitives/meshPrimitiveData.h"
#include "uitoolkit/uiInspector.h"

namespace BreadEditor {
    struct CreatePrimitiveCommand : Command
    {
        explicit CreatePrimitiveCommand(Node *parentNode, MeshPrimitiveType primitiveType);

        ~CreatePrimitiveCommand() override = default;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

        /// The parameter panel outlives execute(): every frame it may have edited the
        /// primitive, and the mesh is rebuilt from whatever it holds.
        void update() override;

    private:
        /// Owned by the command, because the panel keeps editing it long after the call that
        /// created it has returned.
        std::unique_ptr<MeshPrimitiveData> _data;
        UiInspector *_dataInspector = nullptr;
        Node *_parentNode = nullptr;
        SubscriptionHandle _nodeCreatedSubscription;
        SubscriptionHandle _fileSelectedSubscription;
        SubscriptionHandle _nodeSelectedSubscription;
        MeshPrimitiveType _primitiveType;
        std::string _inspectorId;
        unsigned int _nodeId = 0;

        void onNodeCreated(Node *node);

        void destroyInspector();

        void applyData() const;

        [[nodiscard]] static std::unique_ptr<MeshPrimitiveData> createData(MeshPrimitiveType type);
    };
} // BreadEditor
