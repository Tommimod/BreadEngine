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

        void beginFrame(float deltaTime) const;

        void endFrame(float deltaTime) const;

        void setupDefaultCamera();

        Camera3D &getCamera()
        {
            return _camera;
        }

        void loadGameModule(const char *path);

        void unloadGameModule();

        void callGameRender2D(float deltaTime) const;

        void callGameRender3D(float deltaTime) const;

        // Check if point is inside rectangle
        static bool isCollisionPointRec(Vector2 point, Rectangle rec);

        // Check if point is inside rectangle with hole
        static bool isCollisionPointRec(Vector2 point, Rectangle rec, Rectangle subtraction);

        [[nodiscard]] AssetsConfig &getAssetsConfig() { return _fileSystem; };

    private:
        static std::unique_ptr<Engine> _instance;

        AssetsConfig _fileSystem;
        SystemsRegistry _systems;
        Camera3D _camera;
        ModuleLoader *_gameModuleLoader = nullptr;

        typedef void (*GameInitFunc)();

        typedef void (*GameUpdateFunc)(float deltaTime);

        typedef void (*GameRender2DFunc)(float deltaTime);

        typedef void (*GameRender3DFunc)(float deltaTime);

        typedef void (*GameShutdownFunc)();

        GameInitFunc _gameInit = nullptr;
        GameUpdateFunc _gameUpdate = nullptr;
        GameRender2DFunc _gameRender2D = nullptr;
        GameRender3DFunc _gameRender3D = nullptr;
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
