#include "light.h"

#include <type_traits>

#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Light)
    REGISTER_COMPONENT(Light)

    static_assert(std::is_nothrow_move_constructible_v<Light>);
    static_assert(std::is_nothrow_move_assignable_v<Light>);

    Light::Light(Node *owner)
    {
        _owner = owner;
    }

    void Light::copySettings(const Light &other)
    {
        Component::operator=(other);
        lightType = other.lightType;
        color = other.color;
        range = other.range;
        intensity = other.intensity;
        spotAngle = other.spotAngle;
        spotBlend = other.spotBlend;
        shadowSoftness = other.shadowSoftness;
        withShadows = other.withShadows;
    }

    void Light::releaseHandle()
    {
        if (Renderer::isAlive()) Renderer::get().destroyLight(_handle);
        _handle = {};
    }

    Light::Light(const Light &other)
    {
        copySettings(other);
    }

    Light &Light::operator=(const Light &other)
    {
        if (this == &other) return *this;

        releaseHandle();
        copySettings(other);
        return *this;
    }

    Light::Light(Light &&other) noexcept
    {
        copySettings(other);
        _handle = other._handle;
        other._handle = {};
    }

    Light &Light::operator=(Light &&other) noexcept
    {
        if (this == &other) return *this;

        releaseHandle();
        copySettings(other);
        _handle = other._handle;
        other._handle = {};
        return *this;
    }

    void Light::onDestroy()
    {
        releaseHandle();
    }
} // BreadEngine
