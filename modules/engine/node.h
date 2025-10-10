#pragma once

#include "component.h"
#include <algorithm>
#include <vector>
#include "transform.h"
#include "raylib.h"
#include <typeinfo>
#include "iDisposable.h"

namespace BreadEngine
{
    class Node final : public IDisposable
    {
    public:
        Node();
        ~Node();

        Node& setupAsRoot(const std::string &newName);

        Node& setup(const std::string &newName, Node &nextParent);

        void dispose() override;

        void changeParent(Node *nextParent);

        void destroyChild(Node *node);

        void removeAllChildren();

        void update(float deltaTime) const;

        void onFrameStart(float deltaTime) const;

        void onFrameEnd(float deltaTime) const;

        [[nodiscard]] int getChildCount() const;

        [[nodiscard]] Node &getChild(int index) const;

        [[nodiscard]] std::vector<Node*> getAllChilds();

        [[nodiscard]] Node *getParent() const;

        [[nodiscard]] Node &getRoot();

        [[nodiscard]] const std::string &getName() const;

        void setName(const std::string &nextName);

        [[nodiscard]] Transform &getTransform() const;

        [[nodiscard]] int getDeepLevel() const;

        void unparent(Node *node);

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        T *addComponent()
        {
            auto existingComponent = getComponent<T>();
            if (existingComponent && !existingComponent->isAllowMultiple())
            {
                TraceLog(LOG_ERROR, TextFormat("Node with type %s not allow multiple instances", typeid(T).name()));
                return existingComponent;
            }

            T *comp = new T(this);
            components.push_back(comp);
            return comp;
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        T *addComponent(T *comp)
        {
            if (!comp)
            {
                return nullptr;
            }

            auto existingComponent = getComponent<T>();
            if (existingComponent && !existingComponent->isAllowMultiple())
            {
                TraceLog(LOG_ERROR, TextFormat("Node with type %s not allow multiple instances", typeid(T).name()));
                return comp;
            }

            comp->setParent(this);
            components.push_back(comp);
            return comp;
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        T *getComponent() const
        {
            for (Component *comp: components)
            {
                if (auto t = dynamic_cast<T *>(comp))
                {
                    return t;
                }
            }
            return nullptr;
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        std::vector<T *> getComponents() const
        {
            std::vector<T *> result;
            for (Component *comp: components)
            {
                if (auto t = dynamic_cast<T *>(comp))
                {
                    result.push_back(t);
                }
            }
            return result;
        }

        template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
        void removeComponent()
        {
            auto it = std::remove_if(components.begin(), components.end(),
                                     [](Component *comp)
                                     {
                                         if (auto t = dynamic_cast<T *>(comp))
                                         {
                                             delete t;
                                             return true;
                                         }

                                         return false;
                                     });
            components.erase(it, components.end());
        }

    private:
        std::string name;
        std::vector<Node *> childs;
        Node *parent = nullptr;
        std::vector<Component *> components;
        Transform *transform;
    };
} // namespace BreadEngine
