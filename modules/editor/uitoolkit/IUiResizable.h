#pragma once
#include "raylib.h"
#include "uiElement.h"

namespace BreadEditor {
    class IUiResizable
    {
    public:
        typedef enum
        {
            ANY,
            LEFT,
            RIGHT,
            TOP,
            BOTTOM
        } DragSide;

        bool isDebugResizableRectVisible = false;

        void setHorizontalResized(bool resized, DragSide dragSide = ANY);

        void setVerticalResized(bool resized, DragSide dragSide = ANY);

    protected:
        Vector2 prevMousePos = {0.0f, 0.0f};
        DragSide _dragSide = ANY;
        int tricknessInPixel = 6;
        bool isHorizontalResized = false;
        bool isVerticalResized = false;

        void updateResizable(UiElement &uiElement);

    private:
        bool _isPrepared = false;

        static void changeVerticalSize(UiElement &uiElement, bool isDragUpperSide, bool isDragDownSide, float mouseYDelta);

        static void changeHorizontalSize(UiElement &uiElement, bool isDragLeftSide, bool isDragRightSide, float mouseXDelta);

        void drawDebugRect(const Rectangle &subBounds) const;
    };
} // BreadEditor
