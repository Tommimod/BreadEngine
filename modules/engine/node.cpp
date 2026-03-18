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

    Node::~Node() = default;

    Node &Node::setup(const std::string &newName)
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

    void Node::destroy()
    {
        NodeProvider::onNodeDestroyed.invoke(this);
        if (_parent != nullptr)
        {
            _parent->unparent(this);
        }

        for (const auto child: _childs)
        {
            destroyChild(child);
        }

        Engine::nodePool.release(*this);
    }

    void Node::destroyChild(Node *node)
    {
        if (const auto it = std::ranges::find(_childs, node); it != _childs.end())
        {
            _childs.erase(it);
        }

        node->destroy();
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
        if (_parent == nextParent) return;
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

    Node &Node::getChild(const int index) const
    {
        if (index < 0 || index >= static_cast<int>(_childs.size()))
        {
            throw std::out_of_range("Index out of range");
        }

        return *_childs[index];
    }

    std::vector<Node *> Node::getAllChilds()
    {
        return {_childs.begin(), _childs.end()};
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

    std::string Node::serialize(const std::string &filePath) const
    {
        auto rawData = getRawData();
        rawData.IsCopyPipeline = false;
        if (_parent == nullptr)
        {
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

    Node *Node::deserialize(const std::string &filePath)
    {
        const auto rawConfig = YAML::LoadFile(filePath);
        if (rawConfig.IsNull())
        {
            return nullptr;
        }

        const auto data = rawConfig.as<NodeRawData>();
        return NodeProvider::getNode(data.Id);
    }

    Node *Node::createCopyFromNode(const Node &originalNode)
    {
        return createCopyFromData(getDataForCopy(originalNode), *originalNode._parent);
    }

    Node *Node::createCopyFromData(const YAML::Node &dataNode, Node &parent)
    {
        YAML::Emitter out;
        out << dataNode;
        const auto data = out.c_str();
        const auto yamlConfig = YAML::Load(data);
        const auto nextData = yamlConfig.as<NodeRawData>();
        const auto node = NodeProvider::getNode(nextData.Id);
        node->changeParent(&parent);
        return node;
    }

    YAML::Node Node::getDataForCopy(const Node &originalNode)
    {
        auto rawData = originalNode.getRawData();
        rawData.IsCopyPipeline = true;
        return YAML::Node(rawData);
    }

    ComponentsProvider::ComponentMaskArray Node::getComponentMasks(const int maskCount) const
    {
        return ComponentsProvider::GetComponentMasks(_id, maskCount);
    }

    NodeRawData Node::getRawData() const
    {
        std::vector<unsigned int> childIds;
        childIds.reserve(_childs.size());
        for (const auto child: _childs)
        {
            childIds.emplace_back(child->_id);
        }

        return NodeRawData
        {
            .ChildsIds = std::move(childIds),
            .ParentId = _parent != nullptr ? _parent->_id : INT_MAX,
            .Name = _name,
            .Id = _id,
            .IsActive = _isActive
        };
    }
} // namespace BreadEngine
