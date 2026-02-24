#include "engine.h"
#include "moduleLoader.h"
#include <fstream>
#include "nodeProvider.h"
#include <r3d.h>

namespace BreadEngine {
    auto nodeFactory = []() -> Node *
    {
        return new Node();
    };
    ObjectPool<Node> Engine::nodePool(nodeFactory, 10);
    std::unique_ptr<Engine> Engine::_instance = std::make_unique<Engine>();

    Engine::Engine() = default;

    Engine &Engine::getInstance()
    {
        return *_instance;
    }

    Node &Engine::getRootNode()
    {
        static Node _rootNode;
        if (_rootNode.getName().empty()) _rootNode = _rootNode.setupAsRoot("Root");
        return _rootNode;
    }

    float Engine::getDeltaTime()
    {
        return GetFrameTime();
    }

    bool Engine::initialize(const int width, const int height, const char *title)
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
        InitWindow(width, height, title);
        MaximizeWindow();
        R3D_Init(width, height, R3D_FLAG_FXAA);
        SetTargetFPS(60);
        setupDefaultCamera();
        NodeProvider::init();

        return true;
    }

    void Engine::shutdown()
    {
        unloadGameModule();
        R3D_Close();
        CloseWindow();
    }

    bool Engine::shouldClose()
    {
        return WindowShouldClose();
    }

    void Engine::beginFrame() const
    {
        BeginDrawing();

        // Call game logic update if loaded
        if (_gameUpdate)
        {
            _gameUpdate();
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void Engine::endFrame()
    {
        EndDrawing();
    }

    void Engine::setupDefaultCamera()
    {
        _camera.position = (Vector3){0.0f, 10.0f, 10.0f};
        _camera.target = (Vector3){0.0f, 0.0f, 0.0f};
        _camera.up = (Vector3){0.0f, 1.0f, 0.0f};
        _camera.fovy = 45.0f;
        _camera.projection = CAMERA_PERSPECTIVE;
    }

    void Engine::loadGameModule(const char *path)
    {
        unloadGameModule();

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

    void Engine::unloadGameModule()
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

    void Engine::callGameRender2D() const
    {
        if (_gameRender2D)
        {
            _gameRender2D();
        }
    }

    void Engine::callGameRender3D() const
    {
        if (_gameRender3D)
        {
            _gameRender3D();
        }
    }

    std::string Engine::getAssetPath(const std::string &relativePath, const std::string &module)
    {
        std::string basePath = "assets/";

        if (!module.empty())
        {
            basePath += module + "/";
        }

        return basePath + relativePath;
    }

    bool Engine::isFileExists(const std::string &path)
    {
        const std::ifstream file(path);
        return file.good();
    }

    bool Engine::isCollisionPointRec(const Vector2 point, const Rectangle rec)
    {
        return point.x >= rec.x && point.x <= rec.x + rec.width &&
               point.y >= rec.y && point.y <= rec.y + rec.height;
    }

    bool Engine::isCollisionPointRec(const Vector2 point, const Rectangle rec, const Rectangle subtraction)
    {
        const auto inMainRect = isCollisionPointRec(point, rec);
        const auto inSubRect = isCollisionPointRec(point, subtraction);

        return inMainRect && !inSubRect;
    }

    // Реализация утилит движка
    namespace Input {
        bool isKeyPressed(const int key)
        {
            return ::IsKeyPressed(key);
        }

        bool isKeyDown(const int key)
        {
            return ::IsKeyDown(key);
        }

        bool isMouseButtonPressed(const int button)
        {
            return ::IsMouseButtonPressed(button);
        }

        Vector2 getMousePosition()
        {
            return ::GetMousePosition();
        }
    } // namespace Input

    namespace Rendering {
        void drawGrid(const int slices, const float spacing)
        {
            ::DrawGrid(slices, spacing);
        }

        void drawFPS(const int posX, const int posY)
        {
            ::DrawFPS(posX, posY);
        }

        void clearBackground(const Color color)
        {
            ::ClearBackground(color);
        }
    } // namespace Rendering

    namespace Time {
        float getTime()
        {
            return ::GetTime();
        }

        float getFrameTime()
        {
            return ::GetFrameTime();
        }

        int getFPS()
        {
            return ::GetFPS();
        }

        void setTargetFPS(const int fps)
        {
            ::SetTargetFPS(fps);
        }
    } // namespace Time
} // namespace BreadEngine
