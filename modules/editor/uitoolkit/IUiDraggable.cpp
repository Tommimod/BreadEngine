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
        dragSelf(element);
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

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Engine::isCollisionPointRec(mousePos, element->getBounds()))
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
            if (!isDragging && delta > 3)
            {
                isDragging = true;
                onDragStarted.invoke(element);
            }

            if (isDragging && !onlyProvideDragEvents)
            {
                dragSelf(element);
            }
        }

        _lastMousePosition = mousePos;
    }

    void IUiDraggable::disposeDraggable()
    {
        dragContainer = nullptr;
        isDragging = false;
        onlyProvideDragEvents = false;
        _isPrepared = false;
        _mousePositionBeforeClick = Vector2Zero();
        _lastMousePosition = Vector2Zero();
        onDragEnded.unsubscribeAll();
        onDragStarted.unsubscribeAll();
    }

    void IUiDraggable::dragSelf(UiElement *element)
    {
        auto mousePos = GetMousePosition();
        mousePos.x -= element->getSize().x * 0.5f;
        element->setPosition(mousePos);
    }
} // BreadEditor
