#pragma once

extern "C" {
    // Called when game is loaded
    void Game_Initialize();
    
    // Called every frame for game logic
    void Game_Update(float deltaTime);
    
    // Called every frame for 2D rendering
    void Game_Render2D(float deltaTime);
    
    // Called every frame for 3D rendering (inside BeginMode3D/EndMode3D)
    void Game_Render3D(float deltaTime);
    
    // Called when game is unloaded
    void Game_Shutdown();
}
