#pragma once
#include "../../lib/engine/raylib.h"
#include "moduleLoader.h"
#include <memory>
#include "node.h"
#include "objectPool.h"
#include "configs/assetsConfig.h"
#include "systems/core/systemsRegistry.h"

namespace BreadEngine {
    class Engine
    {
    public:
        Engine();

        ~Engine() = default;

        static ObjectPool<Node> nodePool;

        static Engine &getInstance();

        static Node &getRootNode();

        static float getDeltaTime();

        bool initialize(int width, int height, const char *title);

        void shutdown();

        static bool shouldClose();

        void update(float deltaTime) const;

        void fixedUpdate(float fixedDeltaTime) const;

        void onFrameStart(float deltaTime) const;

        void onFrameEnd(float deltaTime) const;

        void loadGameModule(const char *path);

        void unloadGameModule();

        // Check if point is inside rectangle
        static bool isCollisionPointRec(Vector2 point, Rectangle rec);

        // Check if point is inside rectangle with hole
        static bool isCollisionPointRec(Vector2 point, Rectangle rec, Rectangle subtraction);

        [[nodiscard]] AssetsConfig &getAssetsConfig() { return _fileSystem; };

    private:
        static std::unique_ptr<Engine> _instance;

        AssetsConfig _fileSystem;
        SystemsRegistry _systems;
        ModuleLoader *_gameModuleLoader = nullptr;

        typedef void (*GameInitFunc)();

        typedef void (*GameUpdateFunc)(float deltaTime);

        typedef void (*FixedUpdateFunc)(float fixedDeltaTime);

        typedef void (*GameRender2DStartFunc)(float deltaTime);

        typedef void (*GameRender3DStartFunc)(float deltaTime);

        typedef void (*GameRender2DEndFunc)(float deltaTime);

        typedef void (*GameRender3DEndFunc)(float deltaTime);

        typedef void (*GameShutdownFunc)();

        GameInitFunc _gameInit = nullptr;
        GameUpdateFunc _gameUpdate = nullptr;
        FixedUpdateFunc _fixedUpdateFunc = nullptr;
        GameRender2DStartFunc _gameRender2DStart = nullptr;
        GameRender3DStartFunc _gameRender3DStart = nullptr;
        GameRender2DEndFunc _gameRender2DEnd = nullptr;
        GameRender3DEndFunc _gameRender3DEnd = nullptr;
        GameShutdownFunc _gameShutdown = nullptr;

        void initializeSystems();
    };

    namespace Input {
        bool isKeyPressed(int key);

        bool isKeyDown(int key);

        bool isMouseButtonPressed(int button);

        Vector2 getMousePosition();
    } // namespace Input

    namespace Rendering {
        void drawGrid(int slices, float spacing);

        void drawFPS(int posX, int posY);

        void clearBackground(Color color);
    } // namespace Rendering

    namespace Time {
        float getTime();

        float getFrameTime();

        int getFPS();

        void setTargetFPS(int fps);
    } // namespace Time
} // namespace BreadEngine
