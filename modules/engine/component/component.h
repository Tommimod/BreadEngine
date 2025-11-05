#pragma once
#include <string>

namespace BreadEngine
{
    // Forward declaration
    class Node;

    struct Component
    {
        explicit Component(Node *parent);

        virtual ~Component();

        virtual bool isAllowMultiple() { return false; }

        virtual void init();

        virtual void start();

        virtual void update(float deltaTime);

        virtual void fixedUpdate(float fixedDeltaTime);

        virtual void onFrameStart(float deltaTime);

        virtual void onFrameEnd(float deltaTime);

        virtual void destroy();

        [[nodiscard]] virtual std::string toString() const;

        [[nodiscard]] Node *getParent() const;

        [[nodiscard]] bool getIsActive() const;

        virtual void setIsActive(bool nextActive);

        void setParent(Node *nextParent);

    protected:
        Component();

        Node *_parent = nullptr;
        bool _isActive = true;
    };
} // namespace BreadEngine
