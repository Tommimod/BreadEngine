#include "IUiDraggable.h"

#include "editor.h"
#include "raymath.h"
#include "engine.h"

namespace BreadEditor {
    void IUiDraggable::forceStartDrag(UiElement *element)
    {
        const auto root = &Editor::getInstance().mainWindow;
        element->changeParent(root);
        _mousePositionBeforeClick = GetMousePosition();
        _isPrepared = true;
    }

    void IUiDraggable::updateDraggable(UiElement *element)
    {
        const auto mousePos = GetMousePosition();
        if (IsMouseButtonUp(MOUSE_LEFT_BUTTON))
        {
            _mousePositionBeforeClick = Vector2Zero();
            _isPrepared = false;
            if (isDragging)
            {
                onDragEnded.invoke(element);
                isDragging = false;
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && BreadEngine::Engine::isCollisionPointRec(mousePos, element->getBounds()))
        {
            if (_mousePositionBeforeClick.x == 0 && _mousePositionBeforeClick.y == 0)
            {
                _mousePositionBeforeClick = mousePos;
                _isPrepared = true;
            }
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && _isPrepared)
        {
            const auto delta = Vector2Distance(_mousePositionBeforeClick, mousePos);
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

        _lastMousePosition = mousePos;
    }

    void IUiDraggable::dragSelf(UiElement *element)
    {
        auto mousePos = GetMousePosition();
        mousePos.x -= element->getSize().x * 0.5f;
        element->setPosition(mousePos);
    }
} // BreadEditor
