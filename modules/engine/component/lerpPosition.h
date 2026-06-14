#pragma once
#include "core/component.h"

namespace BreadEngine {
    struct LerpPosition final : Component
    {
        Vector3 position{};
        Vector3 backPosition{}; //used for pong mode
        float speed = 0; // used for speed mode
        float duration = 0;

        LerpPosition();

        explicit LerpPosition(Vector3 targetPosition);

        explicit LerpPosition(Vector3 targetPosition, float targetDuration);

        LerpPosition &setDuration(float targetDuration);

        LerpPosition &setPongMode(Vector3 targetBackPosition);

        LerpPosition &setSpeed(float targetSpeed);

    private:
        friend class LerpPositionSystem;

        Vector3 _startPosition{};
        float _progress = 0;
        bool _isPongMode = false;
        bool _isSpeedMode = false;
        bool _isFinished = false;

        INSPECTOR_BEGIN(LerpPosition)
            INSPECT_FIELD(_isPongMode)
            INSPECT_FIELD(_isSpeedMode)
            INSPECT_FIELD_COND(speed, [](const LerpPosition* lerp) {return lerp->_isSpeedMode == true;})
            INSPECT_FIELD(duration)
            INSPECT_FIELD(position)
            INSPECT_FIELD_COND(backPosition, [](const LerpPosition* lerp) {return lerp->_isPongMode == true;})
        INSPECTOR_END()
    };
} // BreadEngine
