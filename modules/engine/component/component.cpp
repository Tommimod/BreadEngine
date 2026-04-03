#include "component.h"
#include <typeinfo>
#include "../node.h"
#include "tracy/Tracy.hpp"

namespace BreadEngine {
#define DEFINE_STATIC_PROPS(ClassName) \
    std::vector<Property> ClassName::inspectorProps_;

    void Component::setOwner(Node *owner)
    {
        this->_owner = owner;
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

    Node *Component::getOwner() const
    {
        return _owner;
    }

    bool Component::getIsActive() const
    {
        return _isActive && _owner->getIsActive();
    }

    void Component::setIsActive(const bool nextActive)
    {
        this->_isActive = nextActive;
    }

    YAML::Node Component::serialize()
    {
        ZoneScoped;
        YAML::Node node(YAML::NodeType::Map);
        node["ComponentType"] = getTypeName();
        for (const auto &prop: getInspectedProperties())
        {
            auto val = prop.get(this);
            node[prop.name] = propertyToYaml(prop, val);
        }
        return node;
    }
} // namespace BreadEngine
