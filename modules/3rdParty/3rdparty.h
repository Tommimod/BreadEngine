#pragma once

#ifdef _WIN32
    #ifdef BUILDING_3RDPARTY_DLL
        #define THIRDPARTY_API __declspec(dllexport)
    #else
        #define THIRDPARTY_API __declspec(dllimport)
    #endif
#else
    #define THIRDPARTY_API __attribute__((visibility("default")))
#endif

// Forward declarations for raylib (actual definitions in raylib.h)
struct Vector3;
struct Camera3D;
struct Color;

// Use raylib types directly
typedef struct Vector3 Vector3;
typedef struct Camera3D Camera3D;
typedef struct Color Color;

// 3rd Party API
extern "C" {
    // Raylib wrapper functions
    THIRDPARTY_API void ThirdParty_InitWindow(int width, int height, const char* title);
    THIRDPARTY_API void ThirdParty_CloseWindow();
    THIRDPARTY_API bool ThirdParty_WindowShouldClose();
    THIRDPARTY_API void ThirdParty_SetTargetFPS(int fps);
    
    THIRDPARTY_API void ThirdParty_BeginDrawing();
    THIRDPARTY_API void ThirdParty_EndDrawing();
    THIRDPARTY_API void ThirdParty_ClearBackground(Color color);
    THIRDPARTY_API void ThirdParty_DrawFPS(int posX, int posY);
    
    THIRDPARTY_API void ThirdParty_BeginMode3D(Camera3D camera);
    THIRDPARTY_API void ThirdParty_EndMode3D();
    THIRDPARTY_API void ThirdParty_DrawGrid(int slices, float spacing);
    
    // Clay UI wrapper functions
    THIRDPARTY_API void ThirdParty_Clay_Initialize();
    THIRDPARTY_API void ThirdParty_Clay_Shutdown();
}
