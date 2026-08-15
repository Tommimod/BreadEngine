#include "../../modules/Engine/Engine.h"
#include "../../lib/engine/raylib.h"
#include "game.h"
#include "rendering/renderer.h"

int main()
{
    BreadEngine::Engine &engine = BreadEngine::Engine::getInstance();

    if (!engine.initialize(800, 600, "Example Game"))
    {
        return -1;
    }

    engine.initializeSystems();

    Game_Initialize();
    const auto camera = new Camera();
    camera->fovy = 45.0f;
    camera->position = {0.0f, 2.0f, 12.0f};
    camera->target = {0.0f, 0.5f, 0.0f};
    camera->up = {0.0f, 1.0f, 0.0f};
    camera->projection = CAMERA_PERSPECTIVE;

    auto &renderer = BreadEngine::Renderer::get();

    while (!BreadEngine::Engine::shouldClose())
    {
        const auto deltaTime = BreadEngine::Engine::getDeltaTime();
        engine.update(deltaTime);
        Game_Update(deltaTime);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        Game_Render2DStart(deltaTime);
        DrawFPS(10, 10);

        renderer.beginScene(BreadEngine::toCameraView(*camera));
        DrawGrid(10, 1.0f);
        Game_Render3DStart(deltaTime);
        engine.onFrameStart(deltaTime);
        engine.onFrameEnd(deltaTime);
        renderer.endScene();

        EndDrawing();
    }

    Game_Shutdown();
    engine.shutdown();
    delete camera;
    return 0;
}
