#include "engine.h"
#include "moduleLoader.h"
#include <fstream>
#include <r3d.h>

#include "nodeProvider.h"
#include "models/reservedFileNames.h"
#include "tracy/Tracy.hpp"
#include "systems/cameraDirectorSystem.h"
#include "systems/cameraSystem.h"
#include "systems/lightSystem.h"
#include "systems/meshRendererSystem.h"
#include "systems/spriteRendererSystem.h"
#include "systems/globalLightSystem.h"
#include "systems/lerpPositionSystem.h"
#include "systems/resetChangedFromEditorSystem.h"
#include "validators/mandatoryProjectFilesValidator.h"

namespace BreadEngine {
    std::unique_ptr<Engine> Engine::_instance = std::make_unique<Engine>();
    Node Engine::_rootNode = {};
    bool Engine::_isShuttingDown = false;
    bool Engine::_isRuntime = false;

    void Engine::initializeSystems()
    {
        _engineSystems
                .addSystem<LightSystem>()
                .addSystem<CameraSystem>()
                .addSystem<CameraDirectorSystem>()
                .addSystem<SpriteRendererSystem>()
                .addSystem<MeshRendererSystem>()
                .addSystem<GlobalLightSystem>()
                .addSystem<LerpPositionSystem>()

                //reset
                .addSystem<ResetChangedFromEditorSystem>();

        _engineSystems.initialize();
    }

    Engine::Engine() = default;

    Engine &Engine::getInstance()
    {
        return *_instance;
    }

    Node &Engine::getRootNode()
    {
        return _rootNode;
    }

    void Engine::setNodeAsRoot(const Node &node)
    {
        _rootNode = node;
    }

    float Engine::getDeltaTime()
    {
        return GetFrameTime();
    }

    std::string Engine::getProjectPath()
    {
        return TextFormat("%s%s\\", GetApplicationDirectory(), "assets\\game");
    }

    bool Engine::initialize(const int width, const int height, const char *title)
    {
        ZoneScoped;
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
        InitWindow(width, height, title);
        MaximizeWindow();
        R3D_Init(width, height);
        SetExitKey(KEY_NULL);

        if (const auto isValid = MandatoryProjectFilesValidator::validateAndInitialize(); !isValid)
        {
            return false;
        }

        NodeProvider::init();
        auto &nodeFileGuid = _projectSettings.startNodeGuid;
        if (nodeFileGuid.empty())
        {
            const auto path = TextFormat("%s%s%s", getProjectPath().c_str(), getRootNode().getName().c_str(), ReservedFileNames::MARKER_NODE);
            nodeFileGuid = _projectSettings.startNodeGuid = _fileSystem.getFileByPath(path)->getGUID();
            _projectSettings.serializeConfig();
        }

        const auto nodeFile = _fileSystem.getFileByGuid(nodeFileGuid);
        const auto node = Node::deserialize(nodeFile->getFullPath());
        if (getRootNode().getName().empty()) setNodeAsRoot(*node);
        return true;
    }

    void Engine::shutdown()
    {
        _isShuttingDown = true;
        ComponentsProvider::setDisposed();
        unloadGameModule();
        _engineSystems.dispose(getDeltaTime());
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
        _engineSystems.update(deltaTime);

        if (_gameUpdate)
        {
            _gameUpdate(deltaTime);
        }
    }

    void Engine::fixedUpdate(const float fixedDeltaTime) const
    {
        ZoneScoped;
        _engineSystems.fixedUpdate(fixedDeltaTime);

        if (_fixedUpdateFunc)
        {
            _fixedUpdateFunc(fixedDeltaTime);
        }
    }

    void Engine::onFrameStart(const float deltaTime) const
    {
        ZoneScoped;
        _engineSystems.startFrame(deltaTime);

        if (_gameRender3DStart && _gameRender2DStart)
        {
            _gameRender3DStart(deltaTime);
            _gameRender2DStart(deltaTime);
        }
    }

    void Engine::onFrameEnd(const float deltaTime) const
    {
        ZoneScoped;
        _engineSystems.endFrame(deltaTime);

        if (_gameRender3DEnd && _gameRender2DEnd)
        {
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

    bool Engine::isCollisionPointRec(const Vector2 point, const Rectangle &rec)
    {
        ZoneScoped;
        return point.x >= rec.x && point.x <= rec.x + rec.width &&
               point.y >= rec.y && point.y <= rec.y + rec.height;
    }

    bool Engine::isCollisionPointRec(const Vector2 point, const Rectangle &rec, const Rectangle &subtraction)
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

        void clearBackground(const ::Color color)
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
