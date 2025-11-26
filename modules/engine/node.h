#pragma once

#include "component/component.h"
#include <algorithm>
#include <vector>
#include <unordered_set>
#include "iDisposable.h"
#include "component/componentsProvider.h"

namespace BreadEngine {
    class ComponentsProvider;

    class Node final : public IDisposable
    {
    public:
        Node();

        explicit Node(unsigned int id);

        ~Node();

        Node &setupAsRoot(const std::string &newName);

        Node &setup(const std::string &newName, Node &nextParent);

        void dispose() override;

        void changeParent(Node *nextParent);

        void setChildFirst(Node *node);

        void destroyChild(Node *node);

        void removeAllChildren();

        [[nodiscard]] bool getIsActive() const;

        [[nodiscard]] unsigned int getId() const;

        void setIsActive(bool nextIsActive);

        [[nodiscard]] int getChildCount() const;

        [[nodiscard]] Node &getChild(int index) const;

        [[nodiscard]] std::vector<Node *> getAllChilds();

        [[nodiscard]] Node *getParent() const;

        [[nodiscard]] Node &getRoot();

        [[nodiscard]] const std::string &getName() const;

        void setName(const std::string &nextName);

        [[nodiscard]] int getDeepLevel() const;

        [[nodiscard]] bool isMyChild(Node *node) const;

        void unparent(Node *node);

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        [[nodiscard]] bool has() const;

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        T &add(T component);

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        T &get();

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        void remove() const;

        void remove(const std::type_index type) const
        {
            ComponentsProvider::remove(_id, type);
        }

    private:
        friend class NodeProvider;

        bool _isActive = true;
        unsigned int _id = 0;
        std::string _name;
        std::vector<Node *> _childs{};
        Node *_parent = nullptr;
    };
} // namespace BreadEngine

namespace BreadEngine {
    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> >
    bool Node::has() const
    {
        return ComponentsProvider::has<T>(_id);
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> >
    T &Node::add(T component)
    {
        if (has<T>())
        {
            return get<T>();
        }

        return ComponentsProvider::add<T>(_id, std::move(component));
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> >
    T &Node::get()
    {
        return ComponentsProvider::get<T>(_id);
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> >
    void Node::remove() const
    {
        if (!has<T>() || typeid(T) == typeid(Transform))
        {
            return;
        }

        ComponentsProvider::remove<T>(_id);
    }
}
