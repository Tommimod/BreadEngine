#define BUILDING_3RDPARTY_DLL
#include "3rdparty.h"

// Include raylib headers
#include "../../lib/engine/raylib.h"

// Include Clay
#define CLAY_IMPLEMENTATION
#include "../../lib/engine/clay.h"

extern "C" {
    // Raylib wrapper implementations
    void ThirdParty_InitWindow(int width, int height, const char* title)
    {
        InitWindow(width, height, title);
    }
    
    void ThirdParty_CloseWindow()
    {
        CloseWindow();
    }
    
    bool ThirdParty_WindowShouldClose()
    {
        return WindowShouldClose();
    }
    
    void ThirdParty_SetTargetFPS(int fps)
    {
        SetTargetFPS(fps);
    }
    
    void ThirdParty_BeginDrawing()
    {
        BeginDrawing();
    }
    
    void ThirdParty_EndDrawing()
    {
        EndDrawing();
    }
    
    void ThirdParty_ClearBackground(Color color)
    {
        ClearBackground(color);
    }
    
    void ThirdParty_DrawFPS(int posX, int posY)
    {
        DrawFPS(posX, posY);
    }
    
    void ThirdParty_BeginMode3D(Camera3D camera)
    {
        BeginMode3D(camera);
    }
    
    void ThirdParty_EndMode3D()
    {
        EndMode3D();
    }
    
    void ThirdParty_DrawGrid(int slices, float spacing)
    {
        DrawGrid(slices, spacing);
    }
    
    // Clay wrapper implementations
    void ThirdParty_Clay_Initialize()
    {
        // Clay initialization if needed
    }
    
    void ThirdParty_Clay_Shutdown()
    {
        // Clay cleanup if needed
    }
}
