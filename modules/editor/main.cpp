#include "editor.h"
#include "../engine/engine.h"
#include "../../lib/engine/raylib.h"

int main()
{
    Engine &engine = Engine::getInstance();

    if (!engine.initialize(1920, 1080, "Bread Engine - Editor"))
    {
        return -1;
    }

    BreadEditor::Editor &editor = BreadEditor::Editor::getInstance();
    if (!editor.initialize())
    {
        engine.shutdown();
        return -1;
    }

    while (!Engine::shouldClose())
    {
        const auto deltaTime = Engine::getDeltaTime();
        engine.beginFrame();
        editor.update(deltaTime);

        ClearBackground(RAYWHITE);

        BeginMode3D(engine.getCamera());
        DrawGrid(1000, 1.0f);
        engine.callGameRender3D();
        editor.render3D(deltaTime);
        EndMode3D();

        engine.callGameRender2D();
        editor.render2D(deltaTime);
        engine.endFrame();
    }

    editor.shutdown();
    engine.shutdown();
    return 0;
}
