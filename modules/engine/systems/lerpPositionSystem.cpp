#include "lerpPositionSystem.h"

#include "lerpPosition.h"
#include "raymath.h"
#include "transform.h"

namespace BreadEngine {
    void LerpPositionSystem::update(Node *node, const float deltaTime) //TODO scene snapshot on game start for restore
    {
        if (!node->has<LerpPosition>()) return;
        auto &lerpPosition = node->get<LerpPosition>();
        auto &transform = node->get<Transform>();
        auto currentPosition = transform.getLocalPosition();
        if (Vector3Equals(lerpPosition._startPosition, Vector3Zero()))
        {
            lerpPosition._startPosition = currentPosition;
        }

        if (lerpPosition._isSpeedMode && lerpPosition.speed <= 0) return;
        if (!lerpPosition._isSpeedMode && lerpPosition.duration <= 0) return;
        if (!lerpPosition._isPongMode && lerpPosition._isFinished) return;

        const auto finishPosition = lerpPosition._isFinished ? lerpPosition.backPosition : lerpPosition.position;
        if (lerpPosition._isPongMode)
        {
            if (lerpPosition._isSpeedMode)
            {
                const auto direction = Vector3Subtract(finishPosition, currentPosition);
                const auto normal = Vector3Normalize(direction);
                currentPosition = Vector3Add(currentPosition, Vector3Scale(normal, lerpPosition.speed * deltaTime));
                lerpPosition._progress = 0;
            }
            else
            {
                lerpPosition._progress += deltaTime;
                currentPosition = Vector3Lerp(lerpPosition._startPosition, finishPosition, lerpPosition._progress);
                if (lerpPosition._progress >= lerpPosition.duration)
                {
                    lerpPosition._isFinished = true;
                    lerpPosition._progress = 0;
                    lerpPosition._startPosition = currentPosition;
                }
            }
        }
        else if (!lerpPosition._isFinished)
        {
            if (lerpPosition._isSpeedMode)
            {
                const auto direction = Vector3Subtract(finishPosition, currentPosition);
                const auto normal = Vector3Normalize(direction);
                currentPosition = Vector3Add(currentPosition, Vector3Scale(normal, lerpPosition.speed * deltaTime));

                const auto startDistance = Vector3Distance(lerpPosition._startPosition, finishPosition);
                const auto currentDistance = Vector3Distance(currentPosition, finishPosition);
                lerpPosition._progress = currentDistance / startDistance;
            }
            else
            {
                lerpPosition._progress += deltaTime;
                currentPosition = Vector3Lerp(lerpPosition._startPosition, finishPosition, lerpPosition._progress);
                if (lerpPosition._progress >= lerpPosition.duration)
                {
                    lerpPosition._isFinished = true;
                }
            }
        }

        transform.setLocalPosition(currentPosition);
        if (Vector3Distance(currentPosition, finishPosition) < 0.001f)
        {
            lerpPosition._isFinished = true;
        }
    }
} // BreadEngine
