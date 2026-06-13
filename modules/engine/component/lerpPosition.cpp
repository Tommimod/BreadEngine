#include "lerpPosition.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(LerpPosition)
    REGISTER_COMPONENT(LerpPosition)

    LerpPosition::LerpPosition() = default;

    LerpPosition::LerpPosition(const Vector3 targetPosition)
    {
        position = targetPosition;
    }

    LerpPosition::LerpPosition(const Vector3 targetPosition, const float targetDuration)
    {
        position = targetPosition;
        duration = targetDuration;
    }

    LerpPosition &LerpPosition::setDuration(const float targetDuration)
    {
        duration = targetDuration;
        return *this;
    }

    LerpPosition &LerpPosition::setPongMode(const Vector3 targetBackPosition)
    {
        _isPongMode = true;
        backPosition = targetBackPosition;
        return *this;
    }

    LerpPosition &LerpPosition::setSpeed(const float targetSpeed)
    {
        _isSpeedMode = true;
        speed = targetSpeed;
        return *this;
    }
} // BreadEngine
