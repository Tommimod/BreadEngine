#include "editor.h"
#include "../engine/engine.h"
#include "../../lib/engine/raylib.h"

int main()
{
    Engine &engine = Engine::GetInstance();

    if (!engine.Initialize(1200, 800, "Bread Engine - Editor"))
    {
        return -1;
    }

    BreadEditor::Editor &editor = BreadEditor::Editor::GetInstance();
    if (!editor.Initialize())
    {
        engine.Shutdown();
        return -1;
    }

    while (!Engine::ShouldClose())
    {
        const auto deltaTime = Engine::GetDeltaTime();
        engine.BeginFrame();
        editor.Update(deltaTime);

        ClearBackground(RAYWHITE);

        BeginMode3D(engine.GetCamera());
        DrawGrid(1000, 1.0f);
        engine.CallGameRender3D();
        EndMode3D();

        engine.CallGameRender2D();
        editor.Render(deltaTime);

        engine.EndFrame();
    }

    editor.Shutdown();
    engine.Shutdown();
    return 0;
}
