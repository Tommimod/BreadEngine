#include "engine.h"
#include "moduleLoader.h"
#include <fstream>
#include "nodeProvider.h"
#include <r3d.h>
#include "tracy/Tracy.hpp"

namespace BreadEngine {
    auto nodeFactory = []() -> Node *
    {
        return new Node();
    };
    ObjectPool<Node> Engine::nodePool(nodeFactory, 10);
    std::unique_ptr<Engine> Engine::_instance = std::make_unique<Engine>();

    void Engine::initializeSystems()
    {
    }

    Engine::Engine() = default;

    Engine &Engine::getInstance()
    {
        return *_instance;
    }

    Node &Engine::getRootNode()
    {
        static Node _rootNode;
        if (_rootNode.getName().empty()) _rootNode = _rootNode.setup("Root");
        return _rootNode;
    }

    float Engine::getDeltaTime()
    {
        return GetFrameTime();
    }

    bool Engine::initialize(const int width, const int height, const char *title)
    {
        ZoneScoped;
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
        InitWindow(width, height, title);
        MaximizeWindow();
        R3D_Init(GetScreenWidth(), GetScreenHeight(), R3D_FLAG_FXAA);
        setupDefaultCamera();
        NodeProvider::init();
        initializeSystems();

        _systems.initialize();
        return true;
    }

    void Engine::shutdown()
    {
        ComponentsProvider::setDisposed();
        unloadGameModule();
        R3D_Close();
        CloseWindow();
    }

    bool Engine::shouldClose()
    {
        return WindowShouldClose();
    }

    void Engine::beginFrame(float deltaTime) const
    {
        ZoneScoped;
        BeginDrawing();

        // Call game logic update if loaded
        if (_gameUpdate)
        {
            _gameUpdate(deltaTime);
        }

        _systems.update(getDeltaTime());
    }

    void Engine::endFrame(float deltaTime) const
    {
        ZoneScoped;
        EndDrawing();
        _systems.endFrame(getDeltaTime());
        FrameMark;
    }

    void Engine::setupDefaultCamera()
    {
        _camera.position = {0.0f, 10.0f, 10.0f};
        _camera.target = {0.0f, 0.0f, 0.0f};
        _camera.up = {0.0f, 1.0f, 0.0f};
        _camera.fovy = 45.0f;
        _camera.projection = CAMERA_PERSPECTIVE;
    }

    void Engine::loadGameModule(const char *path)
    {
        ZoneScoped;
        unloadGameModule();

        _gameModuleLoader = new ModuleLoader();
        if (!_gameModuleLoader->LoadModule(path))
        {
            delete _gameModuleLoader;
            _gameModuleLoader = nullptr;
            Logger::LogError(TextFormat("Failed to load module: %s", path));
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
        ZoneScoped;
        if (!_gameModuleLoader)
        {
            return;
        }

        GameShutdownFunc shutdownFunc = _gameShutdown;
        _gameInit = nullptr;
        _gameUpdate = nullptr;
        _gameRender2D = nullptr;
        _gameRender3D = nullptr;
        _gameShutdown = nullptr;

        if (shutdownFunc)
        {
            Logger::LogInfo("Calling Game_Shutdown...");
            shutdownFunc();
        }

        ModuleLoader* loader = _gameModuleLoader;
        _gameModuleLoader = nullptr;

        Logger::LogInfo("Unloading game module...");
        loader->UnloadModule();
        delete loader;
        Logger::LogInfo("Game module unloaded.");
    }

    void Engine::callGameRender2D(float deltaTime) const
    {
        ZoneScoped;
        if (_gameRender2D)
        {
            _gameRender2D(deltaTime);
        }
    }

    void Engine::callGameRender3D(float deltaTime) const
    {
        ZoneScoped;
        if (_gameRender3D)
        {
            _gameRender3D(deltaTime);
        }
    }

    bool Engine::isCollisionPointRec(const Vector2 point, const Rectangle rec)
    {
        ZoneScoped;
        return point.x >= rec.x && point.x <= rec.x + rec.width &&
               point.y >= rec.y && point.y <= rec.y + rec.height;
    }

    bool Engine::isCollisionPointRec(const Vector2 point, const Rectangle rec, const Rectangle subtraction)
    {
        ZoneScoped;
        const auto inMainRect = isCollisionPointRec(point, rec);
        const auto inSubRect = isCollisionPointRec(point, subtraction);

        return inMainRect && !inSubRect;
    }

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
