#include "game.h"

// Можем использовать raylib напрямую!
#include "../../lib/engine/raylib.h"
#include "../../lib/engine/raymath.h"
#include "../../modules/Engine/Engine.h"
#include <cstdio> // для snprintf
#include <fstream>
#include <string>

static bool gameInitialized = false;
static Vector3 cubePosition = {0.0f, 0.0f, 0.0f};
static float playerSpeed = 5.0f;

extern "C" {
    void Game_Initialize()
    {
        // Initialize game-specific resources
        gameInitialized = true;
        cubePosition = (Vector3){0.0f, 1.0f, 0.0f};
        
        // TODO: Load game config from assets (temporarily disabled)
        // BreadEngine::Engine& engine = BreadEngine::Engine::GetInstance();
        // std::string configPath = engine.GetAssetPath("game_config.txt", "game");
        playerSpeed = 7.5f; // Hardcoded for now
    }
    
    void Game_Update()
    {
        if (!gameInitialized)
            return;
            
        // Game logic only - no rendering here!
        // Rotate the cube
        cubePosition.x = sinf(GetTime()) * 2.0f;
    }
    
    void Game_Render2D()
    {
        if (!gameInitialized)
            return;
            
        // 2D rendering
        DrawText("Example Game Running!", 10, 40, 20, DARKGRAY);
        DrawText("Press ESC to exit", 10, 70, 16, GRAY);
        
        char posText[64];
        snprintf(posText, sizeof(posText), "Cube X: %.2f", cubePosition.x);
        DrawText(posText, 10, 100, 16, BLUE);
        
        char speedText[64];
        snprintf(speedText, sizeof(speedText), "Speed: %.1f (from assets)", playerSpeed);
        DrawText(speedText, 10, 130, 16, GREEN);
    }
    
    void Game_Render3D()
    {
        if (!gameInitialized)
            return;
            
        // 3D rendering - this is called inside BeginMode3D/EndMode3D
        DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
        DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
    }
    
    void Game_Shutdown()
    {
        // Cleanup game resources
        gameInitialized = false;
    }
}