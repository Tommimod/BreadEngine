#include "../../modules/Engine/Engine.h"
#include "../../lib/engine/raylib.h"
#include "game.h"

int main()
{
    BreadEngine::Engine &engine = BreadEngine::Engine::getInstance();

    if (!engine.initialize(800, 600, "Example Game"))
    {
        return -1;
    }

    Game_Initialize();

    while (!BreadEngine::Engine::shouldClose())
    {
        const auto deltaTime = BreadEngine::Engine::getDeltaTime();
        engine.update(deltaTime);
        Game_Update(deltaTime);

        ClearBackground(RAYWHITE);

        Game_Render2DStart(deltaTime);
        DrawFPS(10, 10);

        BeginMode3D(engine.getCamera());
        DrawGrid(10, 1.0f);
        Game_Render3DStart(deltaTime);
        EndMode3D();

        engine.onFrameEnd(deltaTime);
    }

    Game_Shutdown();
    engine.shutdown();
    return 0;
}
