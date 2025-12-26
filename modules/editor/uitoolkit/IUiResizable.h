#pragma once
#include "raylib.h"
#include "uiElement.h"

namespace BreadEditor {
    class IUiResizable
    {
    public:
        bool isDebugResizableRectVisible = false;

        void setHorizontalResized(const bool resized) { isHorizontalResized = resized; }

        void setVerticalResized(const bool resized) { isVerticalResized = resized; }

    protected:
        bool isHorizontalResized = false;
        bool isVerticalResized = false;
        int tricknessInPixel = 6;
        Vector2 prevMousePos = {0.0f, 0.0f};

        void updateResizable(UiElement &uiElement);

    private:
        bool _isPrepared = false;

        static void changeVerticalSize(UiElement &uiElement, bool isDragUpperSide, bool isDragDownSide, float mouseYDelta);

        static void changeHorizontalSize(UiElement &uiElement, bool isDragLeftSide, bool isDragRightSide, float mouseXDelta);

        void drawDebugRect(const Rectangle &subBounds) const;
    };
} // BreadEditor
