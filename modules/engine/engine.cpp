#include "engine.h"
#include "moduleLoader.h"
#include <fstream>

#include "nodeProvider.h"

namespace BreadEngine {
    auto nodeFactory = []() -> Node *
    {
        return new Node();
    };
    ObjectPool<Node> Engine::nodePool(nodeFactory, 10);
    Node Engine::_rootNode;

    Engine::Engine() = default;

    Engine &Engine::GetInstance()
    {
        static Engine instance;
        return instance;
    }

    Node &Engine::getRootNode()
    {
        if (_rootNode.getName().empty()) _rootNode = _rootNode.setupAsRoot("Root");
        return _rootNode;
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
        NodeProvider::init();

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
        if (_gameUpdate)
        {
            _gameUpdate();
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void Engine::EndFrame()
    {
        EndDrawing();
    }

    void Engine::SetupDefaultCamera()
    {
        _camera.position = (Vector3){0.0f, 10.0f, 10.0f};
        _camera.target = (Vector3){0.0f, 0.0f, 0.0f};
        _camera.up = (Vector3){0.0f, 1.0f, 0.0f};
        _camera.fovy = 45.0f;
        _camera.projection = CAMERA_PERSPECTIVE;
    }

    void Engine::LoadGameModule(const char *path)
    {
        UnloadGameModule();

        _gameModuleLoader = new ModuleLoader();
        if (!_gameModuleLoader->LoadModule(path))
        {
            delete _gameModuleLoader;
            _gameModuleLoader = nullptr;
            TraceLog(LOG_ERROR, "Failed to load module: {0}", path);
            return;
        }

        // Get function pointers
        _gameInit = (GameInitFunc) _gameModuleLoader->GetFunction("Game_Initialize");
        _gameUpdate = (GameUpdateFunc) _gameModuleLoader->GetFunction("Game_Update");
        _gameRender2D = (GameRender2DFunc) _gameModuleLoader->GetFunction("Game_Render2D");
        _gameRender3D = (GameRender3DFunc) _gameModuleLoader->GetFunction("Game_Render3D");
        _gameShutdown = (GameShutdownFunc) _gameModuleLoader->GetFunction("Game_Shutdown");

        if (_gameInit)
        {
            _gameInit();
        }
    }

    void Engine::UnloadGameModule()
    {
        if (_gameModuleLoader)
        {
            if (_gameShutdown)
            {
                _gameShutdown();
            }

            delete _gameModuleLoader;
            _gameModuleLoader = nullptr;
            _gameInit = nullptr;
            _gameUpdate = nullptr;
            _gameRender2D = nullptr;
            _gameRender3D = nullptr;
            _gameShutdown = nullptr;
        }
    }

    void Engine::CallGameRender2D() const
    {
        if (_gameRender2D)
        {
            _gameRender2D();
        }
    }

    void Engine::CallGameRender3D() const
    {
        if (_gameRender3D)
        {
            _gameRender3D();
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
        return point.x >= rec.x && point.x <= rec.x + rec.width &&
            point.y >= rec.y && point.y <= rec.y + rec.height;
    }

    bool Engine::IsCollisionPointRec(const Vector2 point, const Rectangle rec, const Rectangle subtraction)
    {
        const auto inMainRect = IsCollisionPointRec(point, rec);
        const auto inSubRect = IsCollisionPointRec(point, subtraction);

        return inMainRect && !inSubRect;
    }

    // Реализация утилит движка
    namespace Input {
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

    namespace Rendering {
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

    namespace Time {
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
