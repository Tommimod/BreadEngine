#include "node.h"
#include <stdexcept>

#include "engine.h"
#include "nodeNotificator.h"

namespace BreadEngine {
    Node::Node()
    {
        childs = std::vector<Node *>();
        components = std::vector<Component *>();
        transform = addComponent<Transform>();
    }

    Node::~Node()
    {
        NodeNotificator::onNodeDestroyed.invoke(this);
        dispose();
    }

    Node &Node::setupAsRoot(const std::string &newName)
    {
        this->name = newName;
        childs = std::vector<Node *>();
        components = std::vector<Component *>();
        transform = addComponent<Transform>();
        NodeNotificator::onNodeCreated.invoke(this);
        return *this;
    }

    Node &Node::setup(const std::string &newName, Node &nextParent)
    {
        this->name = newName;
        childs = std::vector<Node *>();
        components = std::vector<Component *>();
        transform = addComponent<Transform>();
        this->parent = &nextParent;
        this->parent->childs.emplace_back(this);
        NodeNotificator::onNodeCreated.invoke(this);
        return *this;
    }

    void Node::dispose()
    {
        if (isDisposed) return;

        isDisposed = true;
        for (const auto &component: components)
        {
            if (component)
            {
                component->destroy();
            }

            delete component;
        }

        components.clear();
        removeAllChildren();
        parent = nullptr;
    }

    void Node::changeParent(Node *nextParent)
    {
        if (parent != nullptr)
        {
            parent->unparent(this);
        }

        this->parent = nextParent;
        parent->childs.emplace_back(this);
        NodeNotificator::onNodeChangedParent.invoke(this);
    }

    void Node::setChildFirst(Node *node)
    {
        if (const auto it = std::ranges::find(childs, node); it != childs.end())
        {
            childs.erase(it);
        }

        childs.insert(childs.begin(), node);
    }

    void Node::destroyChild(Node *node)
    {
        if (const auto it = std::ranges::find(childs, node); it != childs.end())
        {
            childs.erase(it);
        }

        Engine::nodePool.release(*node);
    }

    void Node::removeAllChildren()
    {
        for (const auto &child: childs)
        {
            if (child)
            {
                Engine::nodePool.release(*child);
            }
        }

        childs.clear();
    }

    void Node::update(const float deltaTime) const
    {
        for (auto &comp: components)
        {
            if (!comp || !comp->getIsActive())
            {
                continue;
            }

            comp->update(deltaTime);
        }
    }

    void Node::onFrameStart(const float deltaTime) const
    {
        for (auto &comp: components)
        {
            if (!comp || !comp->getIsActive())
            {
                continue;
            }

            comp->onFrameStart(deltaTime);
        }
    }

    void Node::onFrameEnd(const float deltaTime) const
    {
        for (auto &comp: components)
        {
            if (!comp || !comp->getIsActive())
            {
                continue;
            }

            comp->onFrameEnd(deltaTime);
        }
    }

    bool Node::getIsActive() const
    {
        return isActive;
    }

    void Node::setIsActive(bool nextIsActive)
    {
        isActive = nextIsActive;
        NodeNotificator::onNodeChangedActive.invoke(this);
    }

    int Node::getChildCount() const
    {
        return static_cast<int>(childs.size());
    }

    std::vector<Node *> Node::getAllChilds()
    {
        return {childs.begin(), childs.end()};
    }

    Node &Node::getChild(const int index) const
    {
        if (index < 0 || index >= static_cast<int>(childs.size()))
        {
            throw std::out_of_range("Index out of range");
        }

        return *childs[index];
    }

    Node *Node::getParent() const
    {
        return parent;
    }

    Node &Node::getRoot()
    {
        if (parent == nullptr || parent == this)
        {
            return *this;
        }

        return parent->getRoot();
    }

    const std::string &Node::getName() const
    {
        return name;
    }

    void Node::setName(const std::string &nextName)
    {
        this->name = nextName;
        NodeNotificator::onNodeRenamed.invoke(this);
    }

    Transform &Node::getTransform() const
    {
        return *transform;
    }

    int Node::getDeepLevel() const
    {
        if (parent == nullptr)
        {
            return 0;
        }

        return parent->getDeepLevel() + 1;
    }

    bool Node::isMyChild(Node *node) const
    {
        auto result = std::ranges::find(childs, node) != childs.end();
        if (!result)
        {
            for (auto child: childs)
            {
                result = child->isMyChild(node);
                if (result) break;
            }
        }

        return result;
    }

    void Node::unparent(Node *node)
    {
        if (const auto it = std::ranges::find(childs, node); it != childs.end())
        {
            childs.erase(it);
        }
    }
} // namespace BreadEngine
