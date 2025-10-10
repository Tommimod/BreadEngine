#include "nodeInstance.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor
{
    NodeInstance::NodeInstance() = default;

    NodeInstance &NodeInstance::setup(const std::string &id, UiElement *parentElement, Node *node)
    {
        this->engineNode = node;
        return dynamic_cast<NodeInstance &>(UiElement::setup(id, parentElement));
    }

    NodeInstance::~NodeInstance() = default;

    void NodeInstance::draw(const float deltaTime)
    {
        GuiButton(bounds, nullptr);
        const auto buttonText = GuiIconText(ICON_CPU, engineNode->getName().c_str());
        GuiLabel(bounds, buttonText);
        UiElement::draw(deltaTime);
    }

    void NodeInstance::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    Node *NodeInstance::getNode() const
    {
        return engineNode;
    }

    void NodeInstance::setParentNode(NodeInstance *nextParentNode)
    {
        this->parentNode = nextParentNode;
    }

    void NodeInstance::setEngineNode(Node *nextEngineNode)
    {
        this->engineNode = nextEngineNode;
    }

    void NodeInstance::deleteSelf()
    {
        UiPool::nodeInstancePool.release(*this);
    }
} // BreadEditor
