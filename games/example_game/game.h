#pragma once

extern "C"
{
// Called when game is loaded
void Game_Initialize();

// Called every frame for game logic
void Game_Update(float deltaTime);

// Called every fixed frame for game/physics logic
void Game_FixedUpdate(float fixedDeltaTime);

// Called every start frame for 2D rendering
void Game_Render2DStart(float deltaTime);

// Called every start frame for 3D rendering
void Game_Render3DStart(float deltaTime);

// Called every end frame for 2D rendering
void Game_Render2DEnd(float deltaTime);

// Called every end frame for 3D rendering
void Game_Render3DEnd(float deltaTime);

// Called when game is unloaded
void Game_Shutdown();
}
