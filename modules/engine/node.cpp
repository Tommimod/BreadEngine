#include "node.h"
#include <stdexcept>
#include <utility>

#include "engine.h"
#include "nodeNotificator.h"

namespace BreadEngine {
    Node::Node()
    {
        _childs = std::vector<Node *>();
        _components = std::vector<Component *>();
        _transform = addComponent<Transform>();
    }

    Node::Node(const int id) : Node()
    {
        this->_id = id;
    }

    Node::~Node()
    {
        NodeNotificator::onNodeDestroyed.invoke(this);
        dispose();
    }

    Node &Node::setupAsRoot(const std::string &newName)
    {
        this->_name = newName;
        _childs = std::vector<Node *>();
        _components = std::vector<Component *>();
        _transform = addComponent<Transform>();
        NodeNotificator::onNodeCreated.invoke(this);
        return *this;
    }

    Node &Node::setup(const std::string &newName, Node &nextParent)
    {
        this->_name = newName;
        _childs = std::vector<Node *>();
        _components = std::vector<Component *>();
        _transform = addComponent<Transform>();
        this->_parent = &nextParent;
        this->_parent->_childs.emplace_back(this);
        NodeNotificator::onNodeCreated.invoke(this);
        return *this;
    }

    void Node::dispose()
    {
        if (_isDisposed) return;

        _isDisposed = true;
        for (const auto &component: _components)
        {
            if (component)
            {
                component->destroy();
            }

            delete component;
        }

        _components.clear();
        removeAllChildren();
        _parent = nullptr;
    }

    void Node::changeParent(Node *nextParent)
    {
        if (_parent != nullptr)
        {
            _parent->unparent(this);
        }

        this->_parent = nextParent;
        _parent->_childs.emplace_back(this);
        NodeNotificator::onNodeChangedParent.invoke(this);
    }

    void Node::setChildFirst(Node *node)
    {
        if (const auto it = std::ranges::find(_childs, node); it != _childs.end())
        {
            _childs.erase(it);
        }

        _childs.insert(_childs.begin(), node);
    }

    void Node::destroyChild(Node *node)
    {
        if (const auto it = std::ranges::find(_childs, node); it != _childs.end())
        {
            _childs.erase(it);
        }

        Engine::nodePool.release(*node);
    }

    void Node::removeAllChildren()
    {
        for (const auto &child: _childs)
        {
            if (child)
            {
                Engine::nodePool.release(*child);
            }
        }

        _childs.clear();
    }

    void Node::update(const float deltaTime) const
    {
        for (auto &comp: _components)
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
        for (auto &comp: _components)
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
        for (auto &comp: _components)
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
        return _isActive;
    }

    void Node::setIsActive(bool nextIsActive)
    {
        _isActive = nextIsActive;
        NodeNotificator::onNodeChangedActive.invoke(this);
    }

    int Node::getChildCount() const
    {
        return static_cast<int>(_childs.size());
    }

    std::vector<Node *> Node::getAllChilds()
    {
        return {_childs.begin(), _childs.end()};
    }

    Node &Node::getChild(const int index) const
    {
        if (index < 0 || index >= static_cast<int>(_childs.size()))
        {
            throw std::out_of_range("Index out of range");
        }

        return *_childs[index];
    }

    Node *Node::getParent() const
    {
        return _parent;
    }

    Node &Node::getRoot()
    {
        if (_parent == nullptr || _parent == this)
        {
            return *this;
        }

        return _parent->getRoot();
    }

    const std::string &Node::getName() const
    {
        return _name;
    }

    void Node::setName(const std::string &nextName)
    {
        this->_name = nextName;
        NodeNotificator::onNodeRenamed.invoke(this);
    }

    Transform &Node::getTransform() const
    {
        return *_transform;
    }

    int Node::getDeepLevel() const
    {
        if (_parent == nullptr)
        {
            return 0;
        }

        return _parent->getDeepLevel() + 1;
    }

    bool Node::isMyChild(Node *node) const
    {
        auto result = std::ranges::find(_childs, node) != _childs.end();
        if (!result)
        {
            for (auto child: _childs)
            {
                result = child->isMyChild(node);
                if (result) break;
            }
        }

        return result;
    }

    void Node::unparent(Node *node)
    {
        if (const auto it = std::ranges::find(_childs, node); it != _childs.end())
        {
            _childs.erase(it);
        }
    }
} // namespace BreadEngine
