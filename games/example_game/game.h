#pragma once

// Game API that must be implemented by user games
extern "C" {
    // Called when game is loaded
    void Game_Initialize();
    
    // Called every frame for game logic
    void Game_Update();
    
    // Called every frame for 2D rendering
    void Game_Render2D();
    
    // Called every frame for 3D rendering (inside BeginMode3D/EndMode3D)
    void Game_Render3D();
    
    // Called when game is unloaded
    void Game_Shutdown();
}
