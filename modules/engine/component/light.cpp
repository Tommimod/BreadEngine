#include "light.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Light)
    REGISTER_COMPONENT(Light)

    Light::Light(Node *owner)
    {
        _owner = owner;
    }

    void Light::setLightType(const R3D_LightType type)
    {
        if (const auto isLightExist = R3D_IsLightExist(_nativeLight); isLightExist && getLightType() != _lightType)
        {
            _lightType = type;
            R3D_DestroyLight(_nativeLight);
            _nativeLight = R3D_CreateLight(_lightType);
        }
        else if (!isLightExist)
        {
            _lightType = type;
            _nativeLight = R3D_CreateLight(_lightType);
        }
    }

    R3D_LightType Light::getLightType() const
    {
        return _lightType;
    }

    void Light::setColor(const Color &color)
    {
        _color = color;
        R3D_SetLightColorV(_nativeLight, _color.asVector3());
    }

    Color Light::getColor() const
    {
        return _color;
    }

    void Light::setWithShadows(const bool withShadows) const
    {
        if (withShadows) R3D_EnableShadow(_nativeLight);
        else R3D_DisableShadow(_nativeLight);
    }

    void Light::setShadowResolution(const int resolution)
    {
        _shadowResolution = resolution;
        setWithShadows(_withShadows);
    }
} // BreadEngine
