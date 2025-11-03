#pragma once
#include "raylib.h"

class CursorSystem
{
public:
    static void setCursor(MouseCursor nextCursor)
    {
        if (nextCursor > cursor)
        {
            cursor = nextCursor;
        }
    }

    static void draw()
    {
        SetMouseCursor(cursor);
        cursor = MOUSE_CURSOR_DEFAULT;
    }

private:
    static MouseCursor cursor;
};
