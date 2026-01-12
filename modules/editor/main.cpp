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

    const auto &viewportWindow = editor.mainWindow.getViewportWindow();
    auto renderTexture = LoadRenderTexture(static_cast<int>(viewportWindow.getViewportSize().width), static_cast<int>(viewportWindow.getViewportSize().height));

    while (!Engine::shouldClose())
    {
        const auto nextWidth = static_cast<int>(viewportWindow.getViewportSize().width);
        const auto nextHeight = static_cast<int>(viewportWindow.getViewportSize().height);
        if (nextWidth != renderTexture.texture.width || nextHeight != renderTexture.texture.height)
        {
            UnloadRenderTexture(renderTexture);
            renderTexture = LoadRenderTexture(nextWidth, nextHeight);
        }

        const auto deltaTime = Engine::getDeltaTime();
        editor.update(deltaTime);

        BeginTextureMode(renderTexture);
        ClearBackground(RAYWHITE);

        BeginMode3D(engine.getCamera());
        DrawGrid(1000, 1.0f);
        engine.callGameRender3D();
        editor.render3D(deltaTime);
        EndMode3D();

        engine.callGameRender2D();
        EndTextureMode();

        engine.beginFrame();
        editor.render2D(renderTexture, deltaTime);
        engine.endFrame();
    }

    editor.shutdown();
    engine.shutdown();
    UnloadRenderTexture(renderTexture);
    return 0;
}
