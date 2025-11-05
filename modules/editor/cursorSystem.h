#pragma once
#include "raylib.h"

class CursorSystem
{
public:
    static void setCursor(const MouseCursor nextCursor)
    {
        if (nextCursor > _cursor)
        {
            _cursor = nextCursor;
        }
    }

    static void draw()
    {
        SetMouseCursor(_cursor);
        _cursor = MOUSE_CURSOR_DEFAULT;
    }

private:
    static MouseCursor _cursor;
};
