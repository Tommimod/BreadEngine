#pragma once
#include "action.h"
#include "uiElement.h"

namespace BreadEditor {
    class IUiDraggable
    {
    public:
        virtual ~IUiDraggable() = default;

        BreadEngine::Action<UiElement *> onDragStarted;
        BreadEngine::Action<UiElement *> onDragEnded;

        UiElement *dragContainer = nullptr;
        bool isDragging = false;
        bool onlyProvideDragEvents = false;

        void forceStartDrag(UiElement *element);

    protected:
        void updateDraggable(UiElement *element);
        void disposeDraggable();

    private:
        Vector2 _mousePositionBeforeClick{};
        Vector2 _lastMousePosition{};
        bool _isPrepared = false;

        static void dragSelf(UiElement *element);
    };
} // BreadEditor
