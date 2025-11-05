#include "ui_manager.h"
#include "clay.h"
#include <raylib.h>
#include <cstdio>
#include <cmath>

extern "C" void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands, Font* fonts);

static void handle_clay_error(Clay_ErrorData errorData) {
    printf("Clay Error: %s\n", errorData.errorText.chars);
}

static Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    Clay_Dimensions textSize = { 0 };

    float maxTextWidth = 0.0f;
    float lineTextWidth = 0;
    int maxLineCharCount = 0;
    int lineCharCount = 0;

    float textHeight = config->fontSize;
    Font* fonts = (Font*)userData;
    Font fontToUse;
    
    // Validate font data before using it
    if (fonts && config->fontId >= 0) {
        fontToUse = fonts[config->fontId];
        // Check if font data is valid and not corrupted
        if (!fontToUse.glyphs || !fontToUse.recs || fontToUse.glyphCount <= 0 || 
            fontToUse.baseSize <= 0 || fontToUse.baseSize > 1000) {
            fontToUse = GetFontDefault();
        }
    } else {
        fontToUse = GetFontDefault();
    }

    float scaleFactor = config->fontSize/(float)fontToUse.baseSize;

    for (int i = 0; i < text.length; ++i, lineCharCount++)
    {
        if (text.chars[i] == '\n') {
            maxTextWidth = fmax(maxTextWidth, lineTextWidth);
            maxLineCharCount = CLAY__MAX(maxLineCharCount, lineCharCount);
            lineTextWidth = 0;
            lineCharCount = 0;
            continue;
        }
        int index = text.chars[i] - 32;
        if (index >= 0 && index < fontToUse.glyphCount && fontToUse.glyphs && fontToUse.recs) {
            // Additional safety check - ensure we're not accessing corrupted memory
            try {
                if (fontToUse.glyphs[index].advanceX != 0) {
                    lineTextWidth += fontToUse.glyphs[index].advanceX;
                } else {
                    lineTextWidth += (fontToUse.recs[index].width + fontToUse.glyphs[index].offsetX);
                }
            } catch (...) {
                // If we get any exception, just use a default character width
                lineTextWidth += 8.0f; // Default character width
            }
        } else {
            // Fallback for invalid characters
            lineTextWidth += 8.0f; // Default character width
        }
    }

    maxTextWidth = fmax(maxTextWidth, lineTextWidth);
    maxLineCharCount = CLAY__MAX(maxLineCharCount, lineCharCount);

    textSize.width = maxTextWidth * scaleFactor + (lineCharCount * config->letterSpacing);
    textSize.height = textHeight;

    return textSize;
}

UIManager::UIManager() : _isInitialized(false), _mousePosition({0, 0}), _mousePressed(false), _mouseReleased(false) {
}

UIManager::~UIManager() {
    if (_isInitialized) {
        shutdown();
    }
}

void UIManager::initialize() {
    if (_isInitialized) return;
    
    // Initialize Clay according to the example
    uint64_t totalMemorySize = Clay_MinMemorySize();
    printf("UIManager: Required memory size: %llu bytes\n", totalMemorySize);
    
    void* arenaMemory = malloc(totalMemorySize);
    if (!arenaMemory) {
        printf("UIManager: Failed to allocate memory for Clay arena\n");
        return;
    }
    
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, arenaMemory);
    
    Clay_Dimensions layoutDimensions = {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    Clay_ErrorHandler errorHandler = {handle_clay_error, nullptr};
    Clay_Initialize(arena, layoutDimensions, errorHandler);
    Font defaultFont = GetFontDefault();
    Clay_SetMeasureTextFunction(Raylib_MeasureText, &defaultFont);
    
    _isInitialized = true;
}

void UIManager::shutdown() {
    if (!_isInitialized) return;
    
    _isInitialized = false;
}

void UIManager::update(float deltaTime) {
    if (!_isInitialized) return;
    
    Vector2 mouseWheelDelta = GetMouseWheelMoveV();
    float mouseWheelX = mouseWheelDelta.x;
    float mouseWheelY = mouseWheelDelta.y;
    
    Clay_Vector2 mousePosition = {GetMousePosition().x, GetMousePosition().y};
    Clay_SetPointerState(mousePosition, IsMouseButtonDown(0));
    Clay_SetLayoutDimensions({static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
    
    Clay_UpdateScrollContainers(true, {mouseWheelX, mouseWheelY}, deltaTime);
}

void UIManager::render_ui(Clay_RenderCommandArray renderCommands, Font* font) {
    if (!_isInitialized) return;
    
    Clay_Raylib_Render(renderCommands, font);
}

void UIManager::handle_mouse_input(Vector2 mousePosition, bool mousePressed, bool mouseReleased) {
    mousePosition = mousePosition;
    mousePressed = mousePressed;
    mouseReleased = mouseReleased;
    
    Clay_Vector2 clayMousePos = {mousePosition.x, mousePosition.y};
    Clay_PointerDataInteractionState state = CLAY_POINTER_DATA_RELEASED;
    if (mousePressed) {
        state = CLAY_POINTER_DATA_PRESSED_THIS_FRAME;
    } else if (mouseReleased) {
        state = CLAY_POINTER_DATA_RELEASED_THIS_FRAME;
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        state = CLAY_POINTER_DATA_PRESSED;
    }
    
    Clay_SetPointerState(clayMousePos, state);
}

void UIManager::begin_layout() {
    if (!_isInitialized) return;
    Clay_BeginLayout();
}

Clay_RenderCommandArray UIManager::end_layout() {
    return Clay_EndLayout();
}

Clay_Color UIManager::raylib_to_clay_color(Color color) {
    return {(float)color.r, (float)color.g, (float)color.b, (float)color.a};
}

Color UIManager::clay_to_raylib_color(Clay_Color color) {
    return {(unsigned char)color.r, (unsigned char)color.g, (unsigned char)color.b, (unsigned char)color.a};
}

