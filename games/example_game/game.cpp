#include "game.h"

#include "../../lib/engine/raylib.h"
#include "../../lib/engine/raymath.h"
#include "../../modules/Engine/Engine.h"
#include <fstream>
#include <string>

static bool gameInitialized = false;
static Vector3 cubePosition = {0.0f, 0.0f, 0.0f};
static float playerSpeed = 5.0f;

extern "C"
{
void Game_Initialize()
{
    gameInitialized = true;
    cubePosition = (Vector3){0.0f, 1.0f, 0.0f};
    playerSpeed = 7.5f;
}

void Game_Update()
{
    if (!gameInitialized) return;

    cubePosition.x = sinf(GetTime()) * 2.0f;
}

void Game_Render2D()
{
    if (!gameInitialized) return;

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
    if (!gameInitialized) return;

    DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
    DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
}

void Game_Shutdown()
{
    gameInitialized = false;
}
}
