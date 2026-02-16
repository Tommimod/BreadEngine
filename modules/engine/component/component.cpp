#include "component.h"
#include <typeinfo>
#include "../node.h"

namespace BreadEngine {
#define DEFINE_STATIC_PROPS(ClassName) \
    std::vector<Property> ClassName::inspectorProps_;

    void Component::setOwner(Node *parent)
    {
        this->_parent = parent;
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
        constexpr auto name = &typeid(this);
        return name->name();
    }

    Node *Component::getParent() const
    {
        return _parent;
    }

    bool Component::getIsActive() const
    {
        return _isActive && _parent->getIsActive();
    }

    void Component::setIsActive(const bool nextActive)
    {
        this->_isActive = nextActive;
    }
} // namespace BreadEngine
