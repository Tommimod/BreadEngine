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
        const auto anchor = uiElement.getAnchor();
        if (isHorizontalResized)
        {
            subBounds.width -= static_cast<float>(tricknessInPixel);
            if (anchor == UI_FIT_RIGHT_VERTICAL)
            {
                subBounds.x += static_cast<float>(tricknessInPixel);
            }
            else
            {
                subBounds.x -= static_cast<float>(tricknessInPixel);
            }
        }

        if (isVerticalResized)
        {
            subBounds.height -= static_cast<float>(tricknessInPixel);
            if (anchor == UI_FIT_BOTTOM_HORIZONTAL || anchor == UI_RIGHT_BOTTOM || anchor == UI_LEFT_BOTTOM)
            {
                subBounds.y += static_cast<float>(tricknessInPixel);
            }
            else
            {
                subBounds.y -= static_cast<float>(tricknessInPixel);
            }
        }

        const auto mousePos = GetMousePosition();
        if (BreadEngine::Engine::isCollisionPointRec(prevMousePos, bounds, subBounds))
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                _isPrepared = true;
            }

            auto isHorizontalResize = false;
            auto isVerticalResize = false;
            if (anchor == UI_FIT_RIGHT_VERTICAL)
            {
                isHorizontalResize = prevMousePos.x >= bounds.x && prevMousePos.x <= subBounds.x;
            }
            else
            {
                isHorizontalResize = prevMousePos.x <= bounds.x + bounds.width && prevMousePos.x >= subBounds.x + subBounds.width;
            }

            if (anchor == UI_FIT_BOTTOM_HORIZONTAL || anchor == UI_RIGHT_BOTTOM || anchor == UI_LEFT_BOTTOM)
            {
                isVerticalResize = prevMousePos.y >= bounds.y && prevMousePos.y <= subBounds.y;
            }
            else
            {
                isVerticalResize = prevMousePos.y <= bounds.y + bounds.height && prevMousePos.y >= subBounds.y + subBounds.height;
            }

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
                    const auto mouseXDelta = prevMousePos.x - mousePos.x;
                    auto size = uiElement.getSize();
                    if (anchor == UI_FIT_RIGHT_VERTICAL)
                    {
                        size.x += mouseXDelta;
                    }
                    else
                    {
                        size.x -= mouseXDelta;
                    }

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
                    const auto mouseYDelta = prevMousePos.y - mousePos.y;
                    auto size = uiElement.getSize();
                    if (anchor == UI_FIT_BOTTOM_HORIZONTAL || anchor == UI_RIGHT_BOTTOM || anchor == UI_LEFT_BOTTOM)
                    {
                        size.y += mouseYDelta;
                    }
                    else
                    {
                        size.y -= mouseYDelta;
                    }

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
