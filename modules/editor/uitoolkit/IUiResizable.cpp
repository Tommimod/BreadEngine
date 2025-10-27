#include "IUiResizable.h"

#include "engine.h"

namespace BreadEditor {
    void IUiResizable::updateResizable(UiElement &uiElement)
    {
        const auto bounds = uiElement.getBounds();
        auto subBounds = bounds;
        if (isHorizontalResized)
        {
            subBounds.width -= static_cast<float>(tricknessInPixel);
            subBounds.x += static_cast<float>(tricknessInPixel);
        }

        if (isVerticalResized)
        {
            subBounds.height -= static_cast<float>(tricknessInPixel);
            subBounds.y += static_cast<float>(tricknessInPixel);
        }

        const auto mousePos = GetMousePosition();
        if (BreadEngine::Engine::IsCollisionPointRec(prevMousePos, bounds, subBounds))
        {
            auto isHorizontalResize = prevMousePos.x >= bounds.x && prevMousePos.x <= subBounds.x;
            auto isVerticalResize = prevMousePos.y >= bounds.y && prevMousePos.y <= subBounds.y;
            if (isHorizontalResize && isHorizontalResized)
            {
                SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    auto mouseXDelta = prevMousePos.x - mousePos.x;
                    auto size = uiElement.getSize();
                    size.x += mouseXDelta;
                    uiElement.setSize(size);
                }
            }
            else if (isVerticalResize && isVerticalResized)
            {
                SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    auto mouseYDelta = prevMousePos.y - mousePos.y;
                    auto size = uiElement.getSize();
                    size.y += mouseYDelta;
                    uiElement.setSize(size);
                }
            }
        }
        else
        {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        prevMousePos = mousePos;
    }
} // BreadEditor
