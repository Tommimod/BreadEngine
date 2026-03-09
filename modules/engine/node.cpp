#include "node.h"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <fstream>
#include <stdexcept>
#include "transform.h"
#include "engine.h"
#include "nodeProvider.h"
#include "models/reservedFileNames.h"
#include "nodeYaml.h"

namespace BreadEngine {
    Node::Node()
    {
        this->_id = NodeProvider::generateId();
        _childs = std::vector<Node *>();
        add<Transform>(Transform(this));
    }

    Node::Node(const unsigned int id)
    {
        this->_id = id;
        _childs = std::vector<Node *>();
        add<Transform>(Transform(this));
    }

    Node::~Node()
    {
        NodeProvider::onNodeDestroyed.invoke(this);
        dispose();
    }

    Node &Node::setupAsRoot(const std::string &newName)
    {
        this->_name = newName;
        _childs = std::vector<Node *>();
        NodeProvider::onNodeCreated.invoke(this);
        return *this;
    }

    Node &Node::setup(const std::string &newName, Node &nextParent)
    {
        this->_name = newName;
        _childs = std::vector<Node *>();
        this->_parent = &nextParent;
        this->_parent->_childs.emplace_back(this);
        NodeProvider::onNodeCreated.invoke(this);
        return *this;
    }

    void Node::dispose()
    {
        if (_isDisposed) return;

        ComponentsProvider::clearOwner(_id);
        removeAllChildren();
        _parent = nullptr;
        _isDisposed = true;
    }

    void Node::changeParent(Node *nextParent)
    {
        if (_parent != nullptr)
        {
            _parent->unparent(this);
        }

        this->_parent = nextParent;
        _parent->_childs.emplace_back(this);
        NodeProvider::onNodeChangedParent.invoke(this);
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

    bool Node::getIsActive() const
    {
        return _isActive;
    }

    unsigned int Node::getId() const
    {
        return _id;
    }

    void Node::setIsActive(bool nextIsActive)
    {
        if (_parent != nullptr)
        {
            nextIsActive = _parent->getIsActive() && nextIsActive;
        }

        _isActive = nextIsActive;
        for (const auto &child: _childs)
        {
            child->setIsActive(nextIsActive);
        }

        NodeProvider::onNodeChangedActive.invoke(this);
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
        NodeProvider::onNodeRenamed.invoke(this);
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
            for (const auto child: _childs)
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

    std::string Node::serialize()
    {
        std::vector<unsigned int> childIds;
        childIds.reserve(_childs.size());
        for (auto child: _childs)
        {
            childIds.emplace_back(child->_id);
        }

        auto rawData = NodeRawData
        {
            .ChildsIds = std::move(childIds),
            .ParentId = _parent != nullptr ? _parent->_id : INT_MAX,
            .Name = _name,
            .Id = _id,
            .IsActive = _isActive,
        };
        if (_parent == nullptr)
        {
            auto &rootFolder = Engine::getInstance().getAssetsConfig().getRootFolder()->getFullPath();
            const auto filePath = std::string(rootFolder) + "\\" + _name + ReservedFileNames::MARKER_NODE;
            auto data = YAML::Node(rawData);
            std::ofstream process(filePath);
            process.clear();
            process << data;
            process.close();
            YAML::Emitter out;
            out << data;
            return out.c_str();
        }

        auto data = YAML::Node(rawData);
        YAML::Emitter out;
        out << data;
        return out.c_str();
    }

    void Node::deserialize()
    {
        if (_parent != nullptr) return;

        auto &rootFolder = Engine::getInstance().getAssetsConfig().getRootFolder()->getFullPath();
        const auto filePath = std::string(rootFolder) + "\\" + _name + ReservedFileNames::MARKER_NODE;
        const auto rawConfig = YAML::LoadFile(filePath);
        if (rawConfig.IsNull())
        {
            return;
        }

        const auto data = rawConfig.as<NodeRawData>();
        _isActive = data.IsActive;
        _id = data.Id;
        _name = data.Name;
    }
} // namespace BreadEngine
