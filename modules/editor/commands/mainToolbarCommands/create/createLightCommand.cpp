#include "createLightCommand.h"

#include "createEmptyNodeCommand.h"
#include "component/light.h"

namespace BreadEditor {
    CreateLightCommand::CreateLightCommand(BreadEngine::Node *parentNode, const BreadEngine::LightType lightType)
    {
        _lightType = lightType;
        _parentNode = parentNode;
    }

    void CreateLightCommand::execute()
    {
        _nodeCreatedSubscription = BreadEngine::NodeProvider::onNodeCreated.subscribe([this](BreadEngine::Node *node) { onNodeCreated(node); });
        merge(std::make_unique<CreateEmptyNodeCommand>(_parentNode));
    }

    void CreateLightCommand::undo()
    {
        BreadEngine::NodeProvider::onNodeCreated.unsubscribe(_nodeCreatedSubscription);
        Command::undo();
    }

    void CreateLightCommand::onNodeCreated(BreadEngine::Node *node)
    {
        BreadEngine::NodeProvider::onNodeCreated.unsubscribe(_nodeCreatedSubscription);
        auto &light = node->add<BreadEngine::Light>();
        light.lightType = _lightType;
        switch (_lightType)
        {
            case BreadEngine::LightType::Directional:
            {
                node->setName("Directional Light");
                break;
            }
            case BreadEngine::LightType::Spot:
            {
                node->setName("Spot Light");
                break;
            }
            case BreadEngine::LightType::Omni:
            {
                node->setName("Omni Light");
                break;
            }
            default: break;
        }
    }
} // BreadEditor
