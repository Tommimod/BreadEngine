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
        editor.callLoop();
    }

    editor.shutdown();
    engine.shutdown();
    return 0;
}
