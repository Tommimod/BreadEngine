#pragma once
#include "raylib.h"
#include "uiElement.h"

namespace BreadEditor {
    class IUiResizable
    {
    protected:
        bool isHorizontalResized = false;
        bool isVerticalResized = false;
        int tricknessInPixel = 6;
        Vector2 prevMousePos = {0.0f, 0.0f};

        void updateResizable(UiElement &uiElement);

    private:
        bool _isPrepared = false;
    };
} // BreadEditor
