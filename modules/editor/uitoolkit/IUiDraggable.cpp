#include "IUiDraggable.h"
#include "raymath.h"
#include "engine.h"

namespace BreadEditor {
    void IUiDraggable::forceStartDrag()
    {
        mousePositionBeforeClick = GetMousePosition();
        isPrepared = true;
    }

    void IUiDraggable::updateDraggable(UiElement *element)
    {
        const auto mousePos = GetMousePosition();
        if (IsMouseButtonUp(MOUSE_LEFT_BUTTON))
        {
            mousePositionBeforeClick = Vector2Zero();
            isPrepared = false;
            if (isDragging)
            {
                onDragEnded.invoke(element);
                isDragging = false;
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && BreadEngine::Engine::IsCollisionPointRec(mousePos, element->getBounds()))
        {
            if (mousePositionBeforeClick.x == 0 && mousePositionBeforeClick.y == 0)
            {
                mousePositionBeforeClick = mousePos;
                isPrepared = true;
            }
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && isPrepared)
        {
            const auto delta = Vector2Distance(mousePositionBeforeClick, mousePos);
            if (!isDragging && delta > 10)
            {
                isDragging = true;
                onDragStarted.invoke(element);
                TraceLog(LOG_INFO, "Drag started");
            }

            if (isDragging && !onlyProvideDragEvents)
            {
                dragSelf(element);
            }
        }

        lastMousePosition = mousePos;
    }

    void IUiDraggable::dragSelf(UiElement *element) const
    {
        const auto mousePos = GetMousePosition();
        const auto delta = Vector2{lastMousePosition.x - mousePos.x, lastMousePosition.y - mousePos.y};
        auto position = element->getPosition();
        element->setPosition({position.x - delta.x, position.y - delta.y});
    }
} // BreadEditor
