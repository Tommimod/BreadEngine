#include "lerpPositionSystem.h"

#include "lerpPosition.h"
#include "raymath.h"
#include "transform.h"

namespace BreadEngine {
    void LerpPositionSystem::update(Node *node, const float deltaTime)
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

                const auto startDistance = Vector3Distance(lerpPosition._startPosition, finishPosition);
                const auto currentDistance = Vector3Distance(currentPosition, finishPosition);
                lerpPosition._progress = 1.0f - (currentDistance / startDistance);
                if (lerpPosition._progress >= 1.0f)
                {
                    lerpPosition._progress = 0;
                    currentPosition = finishPosition;
                }
            }
            else
            {
                lerpPosition._progress += deltaTime;
                const auto progress = lerpPosition._progress / lerpPosition.duration;
                currentPosition = Vector3Lerp(lerpPosition._startPosition, finishPosition, progress);
                if (progress >= 1.0f)
                {
                    lerpPosition._progress = 0;
                    currentPosition = finishPosition;
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
                lerpPosition._progress = 1.0f - (currentDistance / startDistance);
                if (lerpPosition._progress >= 1.0f)
                {
                    currentPosition = finishPosition;
                }
            }
            else
            {
                lerpPosition._progress += deltaTime;
                const auto progress = lerpPosition._progress / lerpPosition.duration;
                currentPosition = Vector3Lerp(lerpPosition._startPosition, finishPosition, progress);
                if (progress >= 1.0f)
                {
                    currentPosition = finishPosition;
                }
            }
        }

        transform.setLocalPosition(currentPosition);
        const auto distance = Vector3Distance(currentPosition, finishPosition);
        if (distance < 0.001f)
        {
            lerpPosition._isFinished = !lerpPosition._isFinished;
            lerpPosition._startPosition = currentPosition;
        }
    }

    void LerpPositionSystem::endOnFrame(Node *node, float deltaTime)
    {
        if (!node->has<LerpPosition>()) return;
        const auto &lerpPosition = node->get<LerpPosition>();
        if (lerpPosition._isFinished && !lerpPosition._isPongMode)
        {
            node->remove<LerpPosition>();
        }
    }
} // BreadEngine
