#include "IUiResizable.h"
#include "../systems/cursorSystem.h"
#include "editor.h"
#include "raymath.h"
#include "engine.h"
#include "uiToolbar.h"

namespace BreadEditor {
    void IUiResizable::updateResizable(UiElement &uiElement)
    {
        if (!isVerticalResized && !isHorizontalResized) return;

        if (IsMouseButtonUp(MOUSE_LEFT_BUTTON))
        {
            _isPrepared = false;
        }

        const auto &bounds = uiElement.getBounds();
        auto subBounds = bounds;
        const auto &pivot = uiElement.getPivot();

        if (isHorizontalResized)
        {
            subBounds.width -= static_cast<float>(tricknessInPixel * 2);
            if (pivot.x == .5f)
            {
                subBounds.x += static_cast<float>(tricknessInPixel) * (uiElement.getPivot().x + .5f);
            }
            else
            {
                subBounds.x += static_cast<float>(tricknessInPixel * 2) * (uiElement.getPivot().x + .5f);
            }
        }

        if (isVerticalResized)
        {
            subBounds.height -= static_cast<float>(tricknessInPixel * 2);
            if (pivot.y == .5f)
            {
                subBounds.y += static_cast<float>(tricknessInPixel) * (uiElement.getPivot().x + .5f);
            }
            else
            {
                subBounds.y += static_cast<float>(tricknessInPixel * 2) * (uiElement.getPivot().x + .5f);
            }
        }

        drawDebugRect(subBounds);
        const auto mousePos = GetMousePosition();
        if (Engine::isCollisionPointRec(prevMousePos, bounds, subBounds))
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                _isPrepared = true;
            }

            auto isHorizontalResize = false;
            auto isVerticalResize = false;
            auto isDragUpperSide = false;
            auto isDragDownSide = false;
            auto isDragLeftSide = false;
            auto isDragRightSide = false;
            if (isHorizontalResized)
            {
                isDragLeftSide = prevMousePos.x >= bounds.x && prevMousePos.x <= subBounds.x;
                isDragRightSide = prevMousePos.x <= bounds.x + bounds.width && prevMousePos.x >= subBounds.x + subBounds.width;
                isHorizontalResize = isDragRightSide || isDragLeftSide;
            }

            if (isVerticalResized)
            {
                isDragUpperSide = prevMousePos.y >= bounds.y && prevMousePos.y <= subBounds.y;
                isDragDownSide = prevMousePos.y <= bounds.y + bounds.height && prevMousePos.y >= subBounds.y + subBounds.height;
                isVerticalResize = isDragUpperSide || isDragDownSide;
            }

            if (isHorizontalResize)
            {
                CursorSystem::setCursor(MOUSE_CURSOR_RESIZE_EW);
            }
            else if (isVerticalResize)
            {
                CursorSystem::setCursor(MOUSE_CURSOR_RESIZE_NS);
            }

            const auto parentElement = uiElement.getParentElement();
            const auto isChildOfContainer = parentElement != nullptr && parentElement->getLayoutType() != LAYOUT_NONE;
            const auto isContainer = uiElement.getLayoutType() != LAYOUT_NONE;
            const auto indexOf = uiElement.getIndex();
            const auto childsCountInParent = parentElement != nullptr ? parentElement->getChildCount() : 0;

            if (isHorizontalResize && _isPrepared)
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    const auto mouseXDelta = prevMousePos.x - mousePos.x;
                    changeHorizontalSize(uiElement, isDragLeftSide, isDragRightSide, mouseXDelta);
                    if (isChildOfContainer && childsCountInParent > 1)
                    {
                        const auto isNeedChangeNext = isDragRightSide && indexOf + 1 != childsCountInParent;
                        const auto isNeedChangePrev = isDragLeftSide && indexOf != 0;
                        if (isNeedChangeNext || isNeedChangePrev)
                        {
                            auto changeElement = isNeedChangeNext ? uiElement.getNextSibling() : nullptr;
                            changeElement = isNeedChangePrev ? uiElement.getPrevSibling() : changeElement;
                            if (changeElement != nullptr)
                            {
                                changeHorizontalSize(*changeElement, isNeedChangeNext, isNeedChangePrev, mouseXDelta);
                            }
                        }
                    }
                    else if (isContainer)
                    {
                        const auto siblings = isDragRightSide
                                                  ? uiElement.getNextSiblingsByEqualHorizontal()
                                                  : uiElement.getPrevSiblingsByEqualHorizontal();
                        for (const auto sibling: siblings)
                        {
                            changeHorizontalSize(*sibling, !isDragLeftSide, !isDragRightSide, mouseXDelta);
                        }
                    }
                }
            }
            else if (isVerticalResize && _isPrepared)
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    const auto mouseYDelta = prevMousePos.y - mousePos.y;
                    changeVerticalSize(uiElement, isDragUpperSide, isDragDownSide, mouseYDelta);

                    if (isChildOfContainer && childsCountInParent > 1)
                    {
                        const auto isNeedChangeNext = isDragDownSide && indexOf + 1 != childsCountInParent;
                        const auto isNeedChangePrev = isDragUpperSide && indexOf != 0;
                        if (isNeedChangeNext || isNeedChangePrev)
                        {
                            auto changeElement = isNeedChangeNext ? uiElement.getNextSibling() : nullptr;
                            changeElement = isNeedChangePrev ? uiElement.getPrevSibling() : changeElement;
                            if (changeElement != nullptr)
                            {
                                changeVerticalSize(*changeElement, isNeedChangeNext, isNeedChangePrev, mouseYDelta * .5f);
                            }
                        }
                    }
                    else if (isContainer)
                    {
                        const auto siblings = isDragUpperSide && pivot.y <= .5f
                                                  ? uiElement.getNextSiblingsByEqualVertical()
                                                  : uiElement.getPrevSiblingsByEqualVertical();
                        for (const auto sibling: siblings)
                        {
                            changeVerticalSize(*sibling, !isDragUpperSide, !isDragDownSide, mouseYDelta * .51f);
                        }
                    }
                }
            }
        }

        prevMousePos = mousePos;
    }

    void IUiResizable::changeVerticalSize(UiElement &uiElement, const bool isDragUpperSide, const bool isDragDownSide, const float mouseYDelta)
    {
        const auto &pivot = uiElement.getPivot();
        const bool isBottomSidePivot = pivot.y > .5f;
        const bool isTopSidePivot = pivot.y < .5f;
        auto size = uiElement.getSize();
        if (isDragUpperSide)
        {
            const auto &currentPos = uiElement.getPosition();
            if (isBottomSidePivot)
            {
                size.y += mouseYDelta;
            }
            else if (!isTopSidePivot)
            {
                size.y += mouseYDelta;
                uiElement.setPosition({currentPos.x, currentPos.y - mouseYDelta * .5f});
            }
            else
            {
                size.y += mouseYDelta;
                uiElement.setPosition({currentPos.x, currentPos.y - mouseYDelta});
            }
        }
        else if (isDragDownSide)
        {
            size.y -= mouseYDelta;
            const auto currentPos = uiElement.getPosition();
            if (isBottomSidePivot)
            {
                size.y -= mouseYDelta;
                uiElement.setPosition({currentPos.x, currentPos.y - mouseYDelta});
            }
            else if (!isTopSidePivot)
            {
                size.y -= mouseYDelta;
                uiElement.setPosition({currentPos.x, currentPos.y - mouseYDelta * .5f});
            }
            else
            {
                size.y -= mouseYDelta;
            }
        }

        const auto parentBounds = uiElement.getParentElement() == nullptr
                                      ? Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}
                                      : uiElement.getParentElement()->getBounds();
        size.y = Clamp(size.y, 100, parentBounds.height);
        uiElement.setSize(size);
        Editor::getInstance().mainWindow.setDirty();
    }

    void IUiResizable::changeHorizontalSize(UiElement &uiElement, const bool isDragLeftSide, const bool isDragRightSide, const float mouseXDelta)
    {
        const auto &pivot = uiElement.getPivot();
        const bool isRightSidePivot = pivot.x > .5f;
        const bool isLeftSidePivot = pivot.x < .5f;
        auto size = uiElement.getSize();
        if (isDragRightSide)
        {
            const auto &currentPos = uiElement.getPosition();
            if (isLeftSidePivot)
            {
                size.x -= mouseXDelta;
            }
            else if (!isRightSidePivot)
            {
                size.x -= mouseXDelta;
                uiElement.setPosition({currentPos.x - mouseXDelta * .5f, currentPos.y});
            }
            else
            {
                size.x -= mouseXDelta;
                uiElement.setPosition({currentPos.x - mouseXDelta, currentPos.y});
            }
        }
        else if (isDragLeftSide)
        {
            const auto currentPos = uiElement.getPosition();
            if (isLeftSidePivot)
            {
                size.x += mouseXDelta;
                uiElement.setPosition({currentPos.x - mouseXDelta, currentPos.y});
            }
            else if (!isRightSidePivot)
            {
                size.x += mouseXDelta;
                uiElement.setPosition({currentPos.x - mouseXDelta * .5f, currentPos.y});
            }
            else
            {
                size.x += mouseXDelta;
            }
        }

        const auto &parentBounds = uiElement.getParentElement() == nullptr
                                       ? Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}
                                       : uiElement.getParentElement()->getBounds();
        size.x = Clamp(size.x, 50, parentBounds.width);
        uiElement.setSize(size);
        Editor::getInstance().mainWindow.setDirty();
    }

    void IUiResizable::drawDebugRect(const Rectangle &subBounds) const
    {
        if (!isDebugResizableRectVisible)
        {
            return;
        }

        auto x = std::to_string(subBounds.x);
        x.erase(x.find_last_not_of('0') + 1, std::string::npos);
        x.erase(x.find_last_not_of('.') + 1, std::string::npos);

        auto y = std::to_string(subBounds.y);
        y.erase(y.find_last_not_of('0') + 1, std::string::npos);
        y.erase(y.find_last_not_of('.') + 1, std::string::npos);

        auto width = std::to_string(subBounds.width);
        width.erase(width.find_last_not_of('0') + 1, std::string::npos);
        width.erase(width.find_last_not_of('.') + 1, std::string::npos);

        auto height = std::to_string(subBounds.height);
        height.erase(height.find_last_not_of('0') + 1, std::string::npos);
        height.erase(height.find_last_not_of('.') + 1, std::string::npos);
        const auto debugText = TextFormat("X:%s Y:%s W:%s H:%s", x.c_str(), y.c_str(), width.c_str(), height.c_str());
        GuiDummyRec(subBounds, debugText);
    }
} // BreadEditor
