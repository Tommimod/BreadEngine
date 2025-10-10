#pragma once

#include "raylib.h"

// Forward declarations
typedef struct Clay_Context Clay_Context;
typedef struct Clay_Color Clay_Color;
typedef struct Clay_ErrorData Clay_ErrorData;
typedef struct Clay_RenderCommandArray Clay_RenderCommandArray;

class UIManager {
public:
    UIManager();
    ~UIManager();

    void initialize();
    void shutdown();
    
    void update(float deltaTime);
    void render();
    void render_ui(Clay_RenderCommandArray renderCommands, Font* font);
    
    void handle_mouse_input(Vector2 mousePosition, bool mousePressed, bool mouseReleased);
    
    void begin_layout();
    Clay_RenderCommandArray end_layout();
    
    Clay_Color raylib_to_clay_color(Color color);
    Color clay_to_raylib_color(Clay_Color color);

private:
    bool isInitialized;
    
    Vector2 mousePosition;
    bool mousePressed;
    bool mouseReleased;
    
};
