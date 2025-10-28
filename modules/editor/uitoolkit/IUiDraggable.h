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
        void forceStartDrag();

    protected:
        void updateDraggable(UiElement *element);

    private:
        bool isPrepared = false;
        Vector2 mousePositionBeforeClick{};
        Vector2 lastMousePosition{};
        void dragSelf(UiElement *element) const;
    };
} // BreadEditor
