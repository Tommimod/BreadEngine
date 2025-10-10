#include "component.h"
#include <typeinfo>

namespace BreadEngine
{
    Component::Component(Node *parent)
    {
        this->parent = parent;
    }

    Component::~Component() = default;

    void Component::init()
    {
    }

    void Component::start()
    {
    }

    void Component::update(float deltaTime)
    {
    }

    void Component::fixedUpdate(float fixedDeltaTime)
    {
    }

    void Component::onFrameStart(float deltaTime)
    {
    }

    void Component::onFrameEnd(float deltaTime)
    {
    }

    void Component::destroy()
    {
    }

    std::string Component::toString() const
    {
        return typeid(this).name();
    }

    Node *Component::getParent() const
    {
        return parent;
    }

    bool Component::getIsActive() const
    {
        return isActive;
    }

    void Component::setIsActive(const bool nextActive)
    {
        this->isActive = nextActive;
    }

    void Component::setParent(Node *nextParent)
    {
        this->parent = nextParent;
    }

    Component::Component() = default;
} // namespace BreadEngine
