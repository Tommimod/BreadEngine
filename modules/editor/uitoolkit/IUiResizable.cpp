#include "IUiResizable.h"

#include "cursorSystem.h"
#include "raymath.h"
#include "engine.h"

namespace BreadEditor {
    void IUiResizable::updateResizable(UiElement &uiElement)
    {
        if (IsMouseButtonUp(MOUSE_LEFT_BUTTON))
        {
            _isPrepared = false;
        }

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
        if (BreadEngine::Engine::isCollisionPointRec(prevMousePos, bounds, subBounds))
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                _isPrepared = true;
            }

            auto isHorizontalResize = prevMousePos.x >= bounds.x && prevMousePos.x <= subBounds.x;
            auto isVerticalResize = prevMousePos.y >= bounds.y && prevMousePos.y <= subBounds.y;
            if (isHorizontalResize && isHorizontalResized)
            {
                CursorSystem::setCursor(MOUSE_CURSOR_RESIZE_EW);
            }
            else if (isVerticalResize && isVerticalResized)
            {
                CursorSystem::setCursor(MOUSE_CURSOR_RESIZE_NS);
            }

            if (isHorizontalResize && isHorizontalResized && _isPrepared)
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    auto mouseXDelta = prevMousePos.x - mousePos.x;
                    auto size = uiElement.getSize();
                    size.x += mouseXDelta;
                    const auto parentBounds = uiElement.getParentElement() == nullptr
                                                  ? Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}
                                                  : uiElement.getParentElement()->getBounds();
                    size.x = Clamp(size.x, 50, parentBounds.width - 50);
                    uiElement.setSize(size);
                }
            }
            else if (isVerticalResize && isVerticalResized && _isPrepared)
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    auto mouseYDelta = prevMousePos.y - mousePos.y;
                    auto size = uiElement.getSize();
                    size.y += mouseYDelta;
                    const auto parentBounds = uiElement.getParentElement() == nullptr
                                                  ? Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}
                                                  : uiElement.getParentElement()->getBounds();
                    size.y = Clamp(size.y, 50, parentBounds.height - 50);
                    uiElement.setSize(size);
                }
            }
        }

        prevMousePos = mousePos;
    }
} // BreadEditor
