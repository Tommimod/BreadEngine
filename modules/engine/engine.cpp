#include "engine.h"
#include "moduleLoader.h"
#include <fstream>
#include "nodeProvider.h"
#include <r3d.h>

#include "systems/cameraDirectorSystem.h"
#include "systems/nodeSystem.h"
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
        _systems.addSystem<NodeSystem>()
                .addSystem<CameraDirectorSystem>();
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
        NodeProvider::init();
        initializeSystems();

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

    void Engine::update(const float deltaTime) const
    {
        ZoneScoped;

        if (_gameUpdate)
        {
            _systems.update(deltaTime);
            _gameUpdate(deltaTime);
        }
    }

    void Engine::fixedUpdate(const float fixedDeltaTime) const
    {
        ZoneScoped;

        if (_fixedUpdateFunc)
        {
            _systems.fixedUpdate(fixedDeltaTime);
            _fixedUpdateFunc(fixedDeltaTime);
        }
    }

    void Engine::onFrameStart(const float deltaTime) const
    {
        ZoneScoped;
        if (_gameRender3DStart && _gameRender2DStart)
        {
            _systems.startFrame(deltaTime);
            _gameRender3DStart(deltaTime);
            _gameRender2DStart(deltaTime);
        }
    }

    void Engine::onFrameEnd(const float deltaTime) const
    {
        ZoneScoped;

        if (_gameRender3DEnd && _gameRender2DEnd)
        {
            _systems.endFrame(deltaTime);
            _gameRender3DEnd(deltaTime);
            _gameRender2DEnd(deltaTime);
        }
        FrameMark;
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
        _fixedUpdateFunc = (FixedUpdateFunc) _gameModuleLoader->GetFunction("Game_FixedUpdate");
        _gameRender2DStart = (GameRender2DStartFunc) _gameModuleLoader->GetFunction("Game_Render2DStart");
        _gameRender3DStart = (GameRender3DStartFunc) _gameModuleLoader->GetFunction("Game_Render3DStart");
        _gameRender2DEnd = (GameRender2DEndFunc) _gameModuleLoader->GetFunction("Game_Render2DEnd");
        _gameRender3DEnd = (GameRender3DEndFunc) _gameModuleLoader->GetFunction("Game_Render3DEnd");
        _gameShutdown = (GameShutdownFunc) _gameModuleLoader->GetFunction("Game_Shutdown");

        if (_gameInit)
        {
            _systems.initialize();
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

        const GameShutdownFunc shutdownFunc = _gameShutdown;
        _gameInit = nullptr;
        _gameUpdate = nullptr;
        _fixedUpdateFunc = nullptr;
        _gameRender2DStart = nullptr;
        _gameRender3DStart = nullptr;
        _gameRender2DEnd = nullptr;
        _gameRender3DEnd = nullptr;
        _gameShutdown = nullptr;

        if (shutdownFunc)
        {
            Logger::LogInfo("Calling Game_Shutdown...");
            shutdownFunc();
        }

        ModuleLoader *loader = _gameModuleLoader;
        _gameModuleLoader = nullptr;

        Logger::LogInfo("Unloading game module...");
        loader->UnloadModule();
        delete loader;
        Logger::LogInfo("Game module unloaded.");
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
