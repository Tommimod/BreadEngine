#include "engine.h"
#include "moduleLoader.h"
#include <fstream>

namespace BreadEngine
{
    auto nodeFactory = []() -> Node* {
        return new Node();
    };
    ObjectPool<Node> Engine::nodePool(nodeFactory, 10);

    Engine &Engine::GetInstance()
    {
        static Engine instance;
        return instance;
    }

    float Engine::GetDeltaTime()
    {
        return GetFrameTime();
    }

    bool Engine::Initialize(const int width, const int height, const char *title)
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
        InitWindow(width, height, title);
        SetTargetFPS(60);

        SetupDefaultCamera();

        return true;
    }

    void Engine::Shutdown()
    {
        UnloadGameModule();
        CloseWindow();
    }

    bool Engine::ShouldClose()
    {
        return WindowShouldClose();
    }

    void Engine::BeginFrame() const
    {
        BeginDrawing();

        // Call game logic update if loaded
        if (gameUpdate)
        {
            gameUpdate();
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void Engine::EndFrame()
    {
        EndDrawing();
    }

    void Engine::SetupDefaultCamera()
    {
        camera.position = (Vector3){0.0f, 10.0f, 10.0f};
        camera.target = (Vector3){0.0f, 0.0f, 0.0f};
        camera.up = (Vector3){0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }

    void Engine::LoadGameModule(const char *path)
    {
        UnloadGameModule();

        gameModuleLoader = new ModuleLoader();
        if (!gameModuleLoader->LoadModule(path))
        {
            delete gameModuleLoader;
            gameModuleLoader = nullptr;
            TraceLog(LOG_ERROR, "Failed to load module: {0}", path);
            return;
        }

        // Get function pointers
        gameInit = (GameInitFunc) gameModuleLoader->GetFunction("Game_Initialize");
        gameUpdate = (GameUpdateFunc) gameModuleLoader->GetFunction("Game_Update");
        gameRender2D = (GameRender2DFunc) gameModuleLoader->GetFunction("Game_Render2D");
        gameRender3D = (GameRender3DFunc) gameModuleLoader->GetFunction("Game_Render3D");
        gameShutdown = (GameShutdownFunc) gameModuleLoader->GetFunction("Game_Shutdown");

        if (gameInit)
        {
            gameInit();
        }
    }

    void Engine::UnloadGameModule()
    {
        if (gameModuleLoader)
        {
            if (gameShutdown)
            {
                gameShutdown();
            }

            delete gameModuleLoader;
            gameModuleLoader = nullptr;
            gameInit = nullptr;
            gameUpdate = nullptr;
            gameRender2D = nullptr;
            gameRender3D = nullptr;
            gameShutdown = nullptr;
        }
    }

    void Engine::CallGameRender2D() const
    {
        if (gameRender2D)
        {
            gameRender2D();
        }
    }

    void Engine::CallGameRender3D() const
    {
        if (gameRender3D)
        {
            gameRender3D();
        }
    }

    std::string Engine::GetAssetPath(const std::string &relativePath, const std::string &module)
    {
        std::string basePath = "assets/";

        if (!module.empty())
        {
            basePath += module + "/";
        }

        return basePath + relativePath;
    }

    bool Engine::FileExists(const std::string &path)
    {
        const std::ifstream file(path);
        return file.good();
    }

    bool Engine::IsCollisionPointRec(const Vector2 point, const Rectangle rec)
    {
        bool collision = false;

        if ((point.x >= rec.x) && (point.x <= (rec.x + rec.width)) &&
            (point.y >= rec.y) && (point.y <= (rec.y + rec.height)))
            collision = true;

        return collision;
    }

    Engine::Engine() : rootNode(Node().setup("Root"))
    {
    }

    // Реализация утилит движка
    namespace Input
    {
        bool IsKeyPressed(const int key)
        {
            return ::IsKeyPressed(key);
        }

        bool IsKeyDown(const int key)
        {
            return ::IsKeyDown(key);
        }

        bool IsMouseButtonPressed(const int button)
        {
            return ::IsMouseButtonPressed(button);
        }

        Vector2 GetMousePosition()
        {
            return ::GetMousePosition();
        }
    } // namespace Input

    namespace Rendering
    {
        void DrawGrid(const int slices, const float spacing)
        {
            ::DrawGrid(slices, spacing);
        }

        void DrawFPS(const int posX, const int posY)
        {
            ::DrawFPS(posX, posY);
        }

        void ClearBackground(const Color color)
        {
            ::ClearBackground(color);
        }
    } // namespace Rendering

    namespace Time
    {
        float GetTime()
        {
            return ::GetTime();
        }

        float GetFrameTime()
        {
            return ::GetFrameTime();
        }

        int GetFPS()
        {
            return ::GetFPS();
        }

        void SetTargetFPS(const int fps)
        {
            ::SetTargetFPS(fps);
        }
    } // namespace Time
} // namespace BreadEngine
