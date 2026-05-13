#include "createEmptyNodeCommand.h"

#include "commands/nodeCommands/createNodeCommand.h"

namespace BreadEditor {
    CreateEmptyNodeCommand::CreateEmptyNodeCommand(BreadEngine::Node *parentNode)
    {
        _engineNode = parentNode;
    }

    CreateEmptyNodeCommand::~CreateEmptyNodeCommand() = default;

    void CreateEmptyNodeCommand::execute()
    {
        auto &nextNode = BreadEngine::NodeProvider::createNode();
        nextNode.setName("Empty node");
        auto data = BreadEngine::Node::getDataForCopy(nextNode);
        BreadEngine::NodeProvider::destroyNode(nextNode);
        merge(std::make_unique<CreateNodeCommand>(_engineNode, std::move(data)));
    }

    void CreateEmptyNodeCommand::undo()
    {
        Command::undo();
    }
} // BreadEditor
