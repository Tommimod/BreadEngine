#include "../../modules/Engine/Engine.h"
#include "../../lib/engine/raylib.h"
#include "game.h"

int main()
{
    BreadEngine::Engine& engine = BreadEngine::Engine::getInstance();
    
    if (!engine.initialize(800, 600, "Example Game"))
    {
        return -1;
    }
    
    Game_Initialize();
    
    while (!engine.shouldClose())
    {
        engine.beginFrame();
        
        Game_Update();
        
        ClearBackground(RAYWHITE);
        
        Game_Render2D();
        DrawFPS(10, 10);
        
        BeginMode3D(engine.getCamera());
        DrawGrid(10, 1.0f);
        Game_Render3D();
        EndMode3D();
        
        engine.endFrame();
    }
    
    Game_Shutdown();
    engine.shutdown();
    return 0;
}
