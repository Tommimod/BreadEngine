#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "../engine/engine.h"
#include "editor.h"
#include "node.h"

namespace BreadEditor
{
    Editor &Editor::GetInstance()
    {
        static Editor instance;
        return instance;
    }

    bool Editor::Initialize()
    {
        if (initialized)
            return true;

        GuiLoadStyleDefault();
        SetTraceLogLevel(LOG_ALL);
        initialized = true;

        const auto rootNode = &Engine::GetInstance().rootNode;
        auto& testNode = Engine::nodePool.get().setup("1", rootNode);
        auto& testNode2 = Engine::nodePool.get().setup("2", &testNode);
        auto& testNode25 = Engine::nodePool.get().setup("2.5", &testNode);
        auto& testNode444 = Engine::nodePool.get().setup("444", &testNode2);
        auto& testNode3 = Engine::nodePool.get().setup("3");
        return true;
    }

    void Editor::Shutdown()
    {
        if (!initialized)
            return;

        CloseProject();
        initialized = false;
    }

    void Editor::Update(float deltaTime)
    {
        if (!initialized)
            return;

        // Update editor logic
    }

    void Editor::Render(float deltaTime)
    {
        if (!initialized)
            return;

        if (GuiButton((Rectangle){10, 50, 120, 30}, "New Project"))
        {
        }

        if (GuiButton((Rectangle){10, 90, 120, 30}, "Open Project"))
        {
        }

        if (IsProjectOpen())
        {
            if (GuiButton((Rectangle){10, 130, 120, 30}, "Compile Game"))
            {
                CompileGame();
            }

            if (GuiButton((Rectangle){10, 170, 120, 30}, "Run Game"))
            {
                RunGame();
            }
        }

        main_window.render(deltaTime);
    }

    bool Editor::CreateProject(const std::string &name, const std::string &path)
    {
        // Create project directory structure
        // Generate basic game template
        currentProjectPath = path + "/" + name;
        return true;
    }

    bool Editor::OpenProject(const std::string &path)
    {
        currentProjectPath = path;
        return true;
    }

    void Editor::CloseProject() { currentProjectPath.clear(); }

    bool Editor::CompileGame()
    {
        if (!IsProjectOpen())
            return false;

        std::string buildCommand = "cd " + currentProjectPath + " && make";
        return system(buildCommand.c_str()) == 0;
    }

    bool Editor::RunGame()
    {
        if (!CompileGame())
            return false;

        std::string gamePath = currentProjectPath + "/build/libgame.dylib";
        Engine::GetInstance().LoadGameModule(gamePath.c_str());
        return true;
    }

    void Editor::StopGame()
    {
        Engine::GetInstance().UnloadGameModule();
    }
} // namespace BreadEditor
