#include "../../modules/Engine/Engine.h"
#include "../../lib/engine/raylib.h"
#include "game.h"

int main()
{
    // Инициализируем движок
    BreadEngine::Engine& engine = BreadEngine::Engine::GetInstance();
    
    if (!engine.Initialize(800, 600, "Example Game"))
    {
        return -1;
    }
    
    // Инициализируем игру
    Game_Initialize();
    
    while (!engine.ShouldClose())
    {
        engine.BeginFrame();
        
        // Обновляем логику игры
        Game_Update();
        
        // Clear background
        ClearBackground(RAYWHITE);
        
        // 2D rendering
        Game_Render2D();
        DrawFPS(10, 10);
        
        // 3D rendering
        BeginMode3D(engine.GetCamera());
        DrawGrid(10, 1.0f);
        Game_Render3D();
        EndMode3D();
        
        engine.EndFrame();
    }
    
    Game_Shutdown();
    engine.Shutdown();
    return 0;
}
