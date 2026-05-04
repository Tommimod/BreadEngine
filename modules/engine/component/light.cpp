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
        if (const auto isLightExist = R3D_IsLightExist(_nativeLight); isLightExist && getLightType() != _settings.lightType)
        {
            _settings.lightType = type;
            R3D_DestroyLight(_nativeLight);
            _nativeLight = R3D_CreateLight(_settings.lightType);
        }
        else if (!isLightExist)
        {
            _settings.lightType = type;
            _nativeLight = R3D_CreateLight(_settings.lightType);
        }
    }

    R3D_LightType Light::getLightType() const
    {
        return _settings.lightType;
    }

    void Light::setColor(const Color &color)
    {
        if (color == _settings.color) return;
        _settings.color = color;
        R3D_SetLightColorV(_nativeLight, _settings.color.asVector3());
    }

    Color Light::getColor() const
    {
        return _settings.color;
    }

    void Light::setWithShadows(const bool withShadows)
    {
        if (_settings.withShadows == withShadows) return;
        _settings.withShadows = withShadows;
        if (withShadows) R3D_EnableShadow(_nativeLight);
        else R3D_DisableShadow(_nativeLight);
    }
} // BreadEngine
